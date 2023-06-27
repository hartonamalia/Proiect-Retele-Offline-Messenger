# Proiect-Retele-Offline-Messenger
# Offline Messenger

The Offline Messenger project is a client/server application developed in C that enables message exchange between connected and offline users. Both connected and offline users have the ability to send messages. Offline users can view messages when they connect to the server.

## Features

- Users can log in to the server using their username and password.
- Once authenticated, users can communicate with each other, reply to specific messages, and log out using the `logout` command.
- Users can send specific replies to received messages and view the conversation history with each user they have communicated with.
- Offline users receive messages upon connecting to the server.
- Messages are stored in a database, allowing users to access message history.
- The application uses TCP (Transmission Control Protocol) for reliable communication between the client and server. TCP ensures maximum connection quality and avoids data loss, making it suitable for message exchange applications.
- The server is implemented using the `fork()` function to handle multiple clients concurrently.
- User registration, login, logout, and conversation history management are handled using the SQLITE relational database management system. SQLITE is compatible with various programming languages and technologies, providing the necessary resources for developing the application efficiently.
- The use of a database offers more efficient data management compared to using a text file.

## Getting Started

To use the "Offline Messenger" application, follow these steps:

1. Create a new account by running the `inregistrare` command and entering a username and password. The account details will be stored in the database with a unique ID.
2. If you already have an account, use the `login` command to enter your credentials. If the credentials are correct, the server will recognize you and grant access to other commands: `sendMessage`, `seeNewMessage`, `reply`, `seeHistory`, `showUsers`, `seeInbox`, and `logout`.
3. Use the `sendMessage` command to send a message. Enter the recipient's username and the desired message. When the recipient logs in and uses the `seeNewMessage` command, they will see the new message.
4. The `seeHistory` command allows you to view the conversation history with a specific user. Enter the username of the person you want to view the conversation history with.
5. The `showUsers` command displays the list of users stored in the database.
6. By accessing the `seeInbox` command, you can see the list of users who have sent you new messages.
7. Finally, use the `logout` command to return to the previous menu. You can then (re)login to access the aforementioned functionalities.

To exit the application and close the connection with the server, use the `exit` command. If you were logged in, the `exit` command will also log you out.

## Environment

The application is developed and tested on Linux using the C programming language and the sqlite3 library.
