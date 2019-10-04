function checkBrowserSupport() {
    // Check if the browser supports these HTML DOM methods.
    return document.getElementById &&
        document.getElementsByTagName &&
        document.createElement &&
        document.createTextNode;
}

function getUrlValue(key) {
    var url = window.location.href;
    var keyIndex = url.indexOf(key) + key.length;
    var valueUrl = url.substring(keyIndex);
    var ampIndex = keyIndex + valueUrl.indexOf("&");
    if (ampIndex > keyIndex) {
        endIndex = ampIndex;
    } else {
        endIndex = url.length;
    }
    var value = url.substring(keyIndex, endIndex);
    return value;
}

/**
 * Get URL parameters as an object
 * @param url string
 */
function getUrlParams(url) {
    // Split url by question mark to get url and params divided
    const splitted = url.split('?');

    // If there are noparams, return empty object
    if (splitted.length < 2) return {};

    // Split by amp to get each url param in an array
    // string [pid=6, search=hej] and so on
    const pairStrings = splitted[1].split('&');

    // Reduce over pair strings to combine each param into a combined params object
    return pairStrings.reduce((pairs, pairString) => {
        // Get key value pair of the parameter
        const pair = pairString.split('=');

        // Update the pairs key of the current param name to the param value
        pairs[pair[0]] = pair[1];

        // Return accumulated pairs object
        return pairs;
    }, {});
}

function goTo(page, guide) {
    switch (page) {
        case "checkTicket":
            window.location.href = "checkTicket?";
            break;
        case "createTicket":
            window.location.href = "createTicket?";
            break;    
        case "generateTickets":
            window.location.href = "generateTickets?";
            break;    
        case "createTicketPre":
            window.location.href = "createTicketPre?";
            break;
        case "aProjectChoose":
            window.location.href = "aProjectChoose?guide=" + guide;
            break;
        case "menu":
            window.location.href = "menu?";
            break;
        case "download":
            window.location.href = "download?";
            break;
        case "useTicket":
            window.location.href = "useTicket?TID=" + guide;
            break;
        default:
            window.location.href = "";
            break;
    }
}

function createButton(body, link, name, guide) {
    var btn = document.createElement("button");
    var text = document.createTextNode(name);

    btn.setAttribute("type", "button");
    btn.setAttribute("id", "buttoon");
    btn.setAttribute("style", "margin-bottom: 1px");
    btn.setAttribute("onclick", "goTo('" + link + "', '" + guide + "')");
    btn.appendChild(text);
    body.appendChild(btn);
}