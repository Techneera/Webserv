const buttons = document.querySelectorAll(".crazy_button");

buttons.forEach(button => {
    button.addEventListener("click", () => {
        switch (button.textContent.trim()) {
            case "GET":
                document.body.style.backgroundColor = "#003344";
                break;

            case "POST":
                document.body.style.backgroundColor = "#33BB99";
                break;

            case "PUT":
                document.body.style.backgroundColor = "#4400AA";
                break;

            case "DELETE":
                document.body.style.backgroundColor = "#000000";
                break;
        }
    });
});

/*
    TODO: Get back on track from here, as now I want to start testing what
            happens with each request method
 */
/* 
const buttons = document.querySelectorAll(".crazy_button");

// Change this to the address and port of YOUR HTTP server.
const serverURL = "http://localhost:8080";

buttons.forEach(button => {
    button.addEventListener("click", async () => {
        const method = button.textContent.trim();

        // Change the background color.
        switch (method) {
            case "GET":
                document.body.style.backgroundColor = "#003344";
                break;

            case "POST":
                document.body.style.backgroundColor = "#33BB99";
                break;

            case "PUT":
                document.body.style.backgroundColor = "#4400AA";
                break;

            case "DELETE":
                document.body.style.backgroundColor = "#000000";
                break;

            default:
                return;
        }

        // Send the HTTP request to your server.
        try {
            const response = await fetch(serverURL, {
                method: method
            });

            const responseBody = await response.text();

            console.log("HTTP Method:", method);
            console.log("Status:", response.status, response.statusText);
            console.log("Response Headers:",
                [...response.headers.entries()]
            );
            console.log("Response Body:", responseBody);

        } catch (error) {
            console.error("Request failed:", error);
        }
    });
});

*/
