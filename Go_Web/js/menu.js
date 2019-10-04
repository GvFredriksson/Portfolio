function createButtons() {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", "/", true);
    xmlhttp.responseType = "json";

    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4 && this.status === 200) {
            var projectButtons = document.getElementById("aMenuForm");
            createButton(projectButtons, "checkTicket", "Check a Ticket");
            var br = document.createElement("br");
            projectButtons.appendChild(br);
            createButton(projectButtons, "createTicket", "Create new Ticket");
            var br = document.createElement("br");
            projectButtons.appendChild(br);
            createButton(projectButtons, "generateTickets", "Generate new Tickets");
            var br = document.createElement("br");
            projectButtons.appendChild(br);
            createButton(projectButtons, "download", "Download QR-codes");
        }
    };
    xmlhttp.send();
}

// With this at the end of the file, the functions will be called when the DOM is ready.
$(document).ready(function () {
    createButtons();
});