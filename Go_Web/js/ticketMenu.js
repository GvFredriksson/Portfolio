function createButtons() {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", "/", true);
    xmlhttp.responseType = "json";
    var RefNum = getUrlParams(window.location.href).refNum
    var Used = getUrlParams(window.location.href).used
    var TID = getUrlParams(window.location.href).TID
    console.log(RefNum)

    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4 && this.status === 200) {
            var ticketH = document.getElementById("ticketMenuForm");
            if(RefNum.length < 1){
                ticketH.innerHTML = "<h2> MISSING </h2><br /><br /> The Ticket does not exist, please enter a different Reference Number<br /><br />"
            }else{
                if(Used === "true"){
                    ticketH.innerHTML = "<h2> USED </h2><br /><br /> Ticket with reference number: "+ RefNum + " has already been used<br /><br />"
                }else{
                    ticketH.innerHTML = "<h2> FREE </h2><br /><br /> Ticket with reference number: "+ RefNum + " is avaliable<br /><br />"
                    var projectButtons = document.getElementById("ticketMenuForm");
                    createButton(projectButtons, "useTicket", "Mark Ticket as Used", TID);
                    var br = document.createElement("br");
                    projectButtons.appendChild(br);
                }
            }
        }
    };
    xmlhttp.send();
}

// With this at the end of the file, the functions will be called when the DOM is ready.
$(document).ready(function () {
    createButtons();
});