function createTicket() {
    var xmlhttp = new XMLHttpRequest();
    var RefNum = getUrlParams(window.location.href).refNum
    xmlhttp.open("POST", "/createTicket?refNum="+RefNum, true);
    xmlhttp.responseType = "json";
    console.log(RefNum)

    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");

    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4 && this.status === 200) {
            var ticket = this.response
            console.log(ticket)
            var ticketH = document.getElementById("createTicket");
            ticketH.innerHTML = "<h2>" + ticket.TID + "</h2><br /><br /> Reference Number: "+ ticket.RefNum + "<br /><br />Last Accessed: " + ticket.LastAcc + 
            "<br /><br />Used: " + ticket.Used + "<br /><br />"
            //createButton(logH, 'use', "Use Ticket",  "&TID="+ticket.TID);
            //var br = document.createElement("br");
            //logH.appendChild(br);
        }
    };
    xmlhttp.send();
}


// With this at the end of the file, the functions will be called when the DOM is ready.
$(document).ready(function () {
    createTicket();
});