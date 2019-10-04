package main

import (
	"archive/zip"
	"database/sql"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	_ "github.com/go-sql-driver/mysql"
	"github.com/gorilla/context"
	"github.com/gorilla/securecookie"
	"github.com/gorilla/sessions"
	qrcode "github.com/skip2/go-qrcode"
	"golang.org/x/crypto/bcrypt"
)

var db *sql.DB

type ticket struct {
	TID     int
	RefNum  string
	LastAcc string
	Used    bool
}

var (
	store = sessions.NewCookieStore(
		securecookie.GenerateRandomKey(64),
		securecookie.GenerateRandomKey(32))
)

const letterBytes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
const (
	letterIdxBits = 6                    // 6 bits to represent a letter index
	letterIdxMask = 1<<letterIdxBits - 1 // All 1-bits, as many as letterIdxBits
	letterIdxMax  = 63 / letterIdxBits   // # of letter indices fitting in 63 bits
)

func generateTickets(res http.ResponseWriter, req *http.Request) {
	if req.Method != "POST" {
		http.ServeFile(res, req, "html/generateTickets.html")
		return
	}
	tmp := req.FormValue("amountTickets")
	amount, err := strconv.Atoi(tmp)
	if err != nil {
		log.Printf("The amount is wrong format, probably")
	}
	for i := 0; i < amount; i++ {
		n := 20
		b := make([]byte, n)
		// A rand.Int63() generates 63 random bits, enough for letterIdxMax letters!
		for i, cache, remain := n-1, rand.Int63(), letterIdxMax; i >= 0; {
			if remain == 0 {
				cache, remain = rand.Int63(), letterIdxMax
			}
			if idx := int(cache & letterIdxMask); idx < len(letterBytes) {
				b[i] = letterBytes[idx]
				i--
			}
			cache >>= letterIdxBits
			remain--
		}
		var err error
		_, err = db.Exec("INSERT INTO tickets (refNum, used) VALUES ('" + string(b) + "', 0)")
		if err != nil {
			panic(err)
		}
	}
	http.Redirect(res, req, "/", 302)
}

func checkCookie(res http.ResponseWriter, req *http.Request) *sessions.Session {
	session, err := store.Get(req, "logged")
	if err != nil {
		log.Printf("Problems with a cookie, make a new one")
		log.Println(err)
		clearSession(res, req)
		http.Redirect(res, req, "/login", 301)
		return nil
	}
	return session
}

func clearSession(res http.ResponseWriter, req *http.Request) {
	session, _ := store.Get(req, "logged")
	out := "logged out"
	log.Println(out)
	session.Values["authenticated"] = false
	session.Values["Admin"] = false
	session.Options.MaxAge = -1
	session.Save(req, res)
}

func loginPage(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Login Page")
	if req.Method != "POST" {
		http.ServeFile(res, req, "html/login.html")
		return
	}

	//-------------cookie-------------------
	session, _ := store.Get(req, "logged")
	//--------------------------------------

	username := req.FormValue("name")
	password := req.FormValue("password")

	var databaseUsername string
	var databasePassword string
	var Admin int

	err := db.QueryRow("SELECT UName, Password, Admin FROM user WHERE UName=?", username).Scan(&databaseUsername, &databasePassword, &Admin)

	if err != nil {
		log.Printf("No user with that name")
		http.Redirect(res, req, "/login", 301)
		return
	}

	err = bcrypt.CompareHashAndPassword([]byte(databasePassword), []byte(password))
	if err != nil {
		log.Printf("Password was wrong")
		http.Redirect(res, req, "/login", 301)
		return
	}

	//-------------cookie-------------------
	session.Values["authenticated"] = true
	session.Values["Admin"] = Admin
	session.Options.MaxAge = 3600
	session.Save(req, res)

	//--------------------------------------
	http.Redirect(res, req, "/", 301)

}

func signupPage(res http.ResponseWriter, req *http.Request) {
	if req.Method != "POST" {
		log.Printf("Signup html")
		http.ServeFile(res, req, "html/signup.html")
		return
	}

	//-------------cookie-------------------
	session, _ := store.Get(req, "logged")
	//--------------------------------------

	username := req.FormValue("name")
	password := req.FormValue("password")
	passwordC := req.FormValue("Confirm password")
	admin := req.FormValue("Admin?")

	if password != passwordC {
		log.Printf("the passwords didn't align")
		http.Redirect(res, req, "signup", 301)
		return
	}

	var user string

	err := db.QueryRow("SELECT UName FROM user WHERE UName=?", username).Scan(&user)

	switch {
	case err == sql.ErrNoRows:
		hashedPassword, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
		if err != nil {
			http.Error(res, "Server error, unable to create your account. 1", 500)
			return
		}
		fmt.Println(admin)
		_, err = db.Exec("INSERT INTO user(UName, Password, Admin) VALUES (?, ?, ?)", username, hashedPassword, true)

		if err != nil {
			http.Error(res, "Server error, unable to create your account. 2", 500)
			return
		}

		//-------------cookie-------------------

		session.Values["authenticated"] = true
		session.Options.MaxAge = 3600
		session.Save(req, res)

		//--------------------------------------

		http.Redirect(res, req, "/", 301)
		return
	case err != nil:
		http.Error(res, "Server error, unable to create your account. 3", 500)
		return
	default:
		http.Redirect(res, req, "/signup", 301)
	}
}

func addTicket(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Adding a new ticket")
	var t = ticket{}

	t.RefNum = req.FormValue("RefNum")
	if req.FormValue("Used") == "yes" {
		t.Used = true
	} else {
		t.Used = false
	}

	var err error
	_, err = db.Exec("INSERT INTO tickets (refNum, used) VALUES ('" + t.RefNum + "', " + strconv.FormatBool(t.Used) + "')")
	if err != nil {
		panic(err)
	}
}

func getTicket(res http.ResponseWriter, req *http.Request) {
	RefNum := req.FormValue("refNum")

	var err error
	var row *sql.Rows
	row, err = db.Query("select * from tickets where refNum = '" + RefNum + "'")
	if err != nil {
		log.Fatal(err)
	}
	defer row.Close()
	var t = ticket{}
	for row.Next() {
		if err = row.Scan(&t.TID, &t.RefNum, &t.LastAcc, &t.Used); err != nil {
			log.Println(err)
			return
		}
	}
	fmt.Println(t)
	output, err := json.Marshal(t)
	if err != nil {
		fmt.Println("error: ", err)
	}

	res.Header().Set("Content-Type", "application/json")
	res.Write(output)
}

func showTicket(res http.ResponseWriter, req *http.Request) {
	if req.Method != "POST" {
		fmt.Println("show Ticket")
		http.ServeFile(res, req, "html/showTicket.html")
		return
	}
	RefNum := req.FormValue("refNum")

	var err error
	var row *sql.Rows
	row, err = db.Query("select TID, refNum, used from tickets where refNum = '" + RefNum + "'")
	if err != nil {
		log.Fatal(err)
	}
	defer row.Close()
	var t = ticket{}
	for row.Next() {
		if err = row.Scan(&t.TID, &t.RefNum, &t.Used); err != nil {
			log.Println(err)
			return
		}
	}
	fmt.Println(t)
	http.Redirect(res, req, "/ticketMenu?refNum="+t.RefNum+"&TID="+strconv.Itoa(t.TID)+"&used="+strconv.FormatBool(t.Used), 302)
}

func showTicketQR(res http.ResponseWriter, req *http.Request) {
	RefNum := req.FormValue("refNum")
	fmt.Println(RefNum)

	var err error
	var row *sql.Rows
	row, err = db.Query("select TID, refNum, used from tickets where refNum = '" + RefNum + "'")
	if err != nil {
		log.Fatal(err)
	}
	defer row.Close()
	var t = ticket{}
	for row.Next() {
		if err = row.Scan(&t.TID, &t.RefNum, &t.Used); err != nil {
			log.Println(err)
			return
		}
	}
	fmt.Println(t)
	http.Redirect(res, req, "/ticketMenu?refNum="+t.RefNum+"&TID="+strconv.Itoa(t.TID)+"&used="+strconv.FormatBool(t.Used), 302)
}

func createTicket(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Inserting ticket")
	if req.Method != "POST" {
		http.ServeFile(res, req, "html/createTicket.html")
		return
	}
	var ticket = ticket{}
	ticket.RefNum = req.FormValue("refNum")
	ticket.Used = false
	fmt.Println(ticket.RefNum)
	var err error
	_, err = db.Exec("INSERT INTO tickets (refnum, used) VALUES ('" + ticket.RefNum + "', '0')")
	if err != nil {
		panic(err)
	}
	http.Redirect(res, req, "/", 302)
}

func menu(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Menu")
	http.ServeFile(res, req, "html/menu.html")
}

func ticketMenu(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Ticket Menu")
	http.ServeFile(res, req, "html/ticketMenu.html")
}

func useTicket(res http.ResponseWriter, req *http.Request) {
	fmt.Println("Use Ticket")
	//http.ServeFile(res, req, "html/ticketMenu.html")
	TID := req.FormValue("TID")

	_, err := db.Exec("UPDATE tickets SET Used = true where TID = '" + TID + "'")
	if err != nil {
		panic(err)
	}
	http.Redirect(res, req, "/", 302)

}

//func generateQR(res http.ResponseWriter, req *http.Request) {
func generateQR(start int, amount int) {

	var err error
	var row *sql.Rows
	row, err = db.Query("SELECT TID, refNum, used FROM tickets ORDER BY TID LIMIT " + strconv.Itoa(start) + ", " + strconv.Itoa(amount) + "")
	if err != nil {
		log.Fatal(err)
	}
	defer row.Close()
	var tickets = []ticket{}
	for row.Next() {
		t := ticket{}
		if err = row.Scan(&t.TID, &t.RefNum, &t.Used); err != nil {
			log.Println(err)
			return
		}
		tickets = append(tickets, t)
	}
	for index, element := range tickets {
		fmt.Println(element.RefNum)
		filename := "png/" + strconv.Itoa(index) + "-" + element.RefNum + ".png"
		url := "https://gvfredriksson.com/checkTicketQR?refNum=" + element.RefNum
		if err := qrcode.WriteFile(url, qrcode.Medium, 256, filename); err != nil {
			if err = os.Remove(filename); err != nil {
				fmt.Printf("Couldn't remove file")
			}
		}
	}
}

//---------------------------------------------------------------------ZIP FILES-----------------------------------------------

func zipit(source, target string) error {
	fmt.Printf("ZIPPING\n")
	zipfile, err := os.Create(target)
	if err != nil {
		return err
	}
	defer zipfile.Close()
	archive := zip.NewWriter(zipfile)
	defer archive.Close()
	info, err := os.Stat(source)
	if err != nil {
		return nil
	}
	var baseDir string
	if info.IsDir() {
		baseDir = filepath.Base(source)
	}

	filepath.Walk(source, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		header, err := zip.FileInfoHeader(info)
		if err != nil {
			return err
		}
		if baseDir != "" {
			header.Name = filepath.Join(baseDir, strings.TrimPrefix(path, source))
		}
		if info.IsDir() {
			header.Name += "/"
		} else {
			header.Method = zip.Deflate
		}
		writer, err := archive.CreateHeader(header)
		if err != nil {
			return err
		}
		if info.IsDir() {
			return nil
		}
		file, err := os.Open(path)
		if err != nil {
			return err
		}
		defer file.Close()
		_, err = io.Copy(writer, file)
		return err
	})

	return err
}

//----------------------------------------------DOWNLOAD----------------------------------------------------------

func download(res http.ResponseWriter, req *http.Request) {
	if req.Method != "POST" {
		http.ServeFile(res, req, "html/download.html")
		return
	}
	start, err := strconv.Atoi(req.FormValue("start"))
	amount, erro := strconv.Atoi(req.FormValue("amount"))
	if err != nil || erro != nil {
		log.Println(err)
		log.Println(erro)
	}
	generateQR(start, amount)
	zipit("png", "files/qrcodes.zip")
	http.ServeFile(res, req, "files/qrcodes.zip")

	directory := "png/"
	// Open the directory and read all its files.
	dirRead, _ := os.Open(directory)
	dirFiles, _ := dirRead.Readdir(0)

	// Loop over the directory's files.
	for index := range dirFiles {
		fileHere := dirFiles[index]
		nameHere := fileHere.Name()
		fullPath := directory + nameHere

		// Remove the file.
		os.Remove(fullPath)
		fmt.Println("Removed file:", fullPath)
	}

	directory = "files/"
	// Open the directory and read all its files.
	dirRead, _ = os.Open(directory)
	dirFiles, _ = dirRead.Readdir(0)

	// Loop over the directory's files.
	for index := range dirFiles {
		fileHere := dirFiles[index]
		nameHere := fileHere.Name()
		fullPath := directory + nameHere

		// Remove the file.
		os.Remove(fullPath)
		fmt.Println("Removed file:", fullPath)
	}
}

//func Handler(w http.ResponseWriter, r *http.Request) {
func main() {

	//-------------------------------------Connect to database-----------------------------
	var err error
	db, err = sql.Open("mysql", "###") // Remover pass and IP
	if err != nil {
		fmt.Println("main: error in handshake")
		panic(err.Error())
	}

	err = db.Ping()
	if err != nil {
		fmt.Println("main error in ping, you are not connected to the database")
		panic(err.Error())
	}

	var unauthMux = http.NewServeMux()
	var authMux = http.NewServeMux()
	//var adminMux = http.NewServeMux()
	var router = http.NewServeMux()

	router.HandleFunc("/", func(res http.ResponseWriter, req *http.Request) {
		activeMux := unauthMux
		session, err := store.Get(req, "logged")
		if err == nil {
			if _, ok := session.Values["authenticated"]; ok {
				activeMux = authMux
			}
			ok := session.Values["Admin"]
			if ok == 1 {
				fmt.Println("Admin user")
				//activeMux = adminMux
			}
		}
		handleFunc, _ := activeMux.Handler(req)
		handleFunc.ServeHTTP(res, req)
	})

	cssHandler := http.FileServer(http.Dir("css/"))
	imagesHandler := http.FileServer(http.Dir("images/"))
	jsHandler := http.FileServer(http.Dir("js/"))

	router.Handle("/css/", http.StripPrefix("/css/", cssHandler))
	router.Handle("/images/", http.StripPrefix("/images/", imagesHandler))
	router.Handle("/js/", http.StripPrefix("/js/", jsHandler))

	//-----------------------------------------http------------------------------------------------

	unauthMux.HandleFunc("/", loginPage)
	unauthMux.HandleFunc("/signup", signupPage)
	//unauthMux.HandleFunc("/signup", aNewUser)

	//-----------------------------------------Authorised------------------------------------------

	authMux.HandleFunc("/", menu)
	authMux.HandleFunc("/showTicket", showTicket)
	authMux.HandleFunc("/checkTicket", showTicket)
	authMux.HandleFunc("/ticketMenu", ticketMenu)
	authMux.HandleFunc("/useTicket", useTicket)
	authMux.HandleFunc("/createTicket", createTicket)

	authMux.HandleFunc("/checkTicketQR", showTicketQR)
	authMux.HandleFunc("/generateTickets", generateTickets)

	authMux.HandleFunc("/download", download)

	//row, err = db.Query("SELECT refNum FROM tickets ORDER BY TID DESC LIMIT '10','10'")
	//generateQR()
	//zipit("png", "files/qrcodes.zip")

	// ------------------------------ Listen and Serve --------------------------------------------
	http.ListenAndServe(":8080", context.ClearHandler(router))

}

