# Backend_server

##About
This game was developed by me in the course "C++ for the backend". This is a web server built using a REST API that can receive and asynchronously process incoming requests. Since this is an online game, it was necessary to add sessions. The user logs in and connects to the game. User information is collected in a PostgerSQL database. Unit tests were also written for some parts of the code.

##How to run?

```bash
git clone git@github.com:ST3PAN0V/backend_server.git
cd backend_server
sudo docker build -t stepanov_server .
sudo docker run --rm -p 80:8080 my_http_server
```
Server has been started. Than go to https://127.0.0.1/

Have fun!!!

