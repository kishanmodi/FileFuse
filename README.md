# COMP-8567 Project: Client-Server File Retrieval System

## Introduction
This project involves the development of a client-server system where clients can request files from a server, and the server retrieves and returns the requested files. The system supports multiple clients connecting from different machines and communicating with the server through sockets.

## Team Members
- [Kishan Modi](https://github.com/KishanModi)
- [Meet Patel](https://github.com/meet57)

## Project Structure
The project consists of the following components:
1. **Server (`serverw24.c`):** Handles client connections and file retrieval requests.
2. **Client (`clientw24.c`):** Sends requests to the server and receives files.
3. **Mirror 1 (`mirror1.c`):** Alternate server instance for handling client connections.
4. **Mirror 2 (`mirror2.c`):** Another alternate server instance.

## Instructions
- The server, Mirror 1, and Mirror 2 must run on different machines or terminals.
- Clients can connect to the server using specified commands to request files.
- Each client command is defined and must be processed accordingly by the server.

## Usage
1. **Server (`serverw24.c`):**
   - Compile: `gcc -o serverw24 serverw24.c`
   - Run: `./serverw24`

2. **Client (`clientw24.c`):**
   - Compile: `gcc -o clientw24 clientw24.c`
   - Run: `./clientw24`

3. **Mirror 1 (`mirror1.c`):**
   - Compile: `gcc -o mirror1 mirror1.c`
   - Run: `./mirror1`

4. **Mirror 2 (`mirror2.c`):**
   - Compile: `gcc -o mirror2 mirror2.c`
   - Run: `./mirror2`

## Client Commands
- `dirlist -a`: List subdirectories alphabetically.
- `dirlist -t`: List subdirectories by creation time.
- `w24fn filename`: Retrieve file information.
- `w24fz size1 size2`: Retrieve files within a size range.
- `w24ft <extension list>`: Retrieve files by extension(s).
- `w24fdb date`: Retrieve files created before a specified date.
- `w24fda date`: Retrieve files created after a specified date.
- `quitc`: Terminate the client process.


## License
[GNU General Public License v3.0](https://raw.githubusercontent.com/kishanmodi/ASP-Final-Project/main/LICENSE?token=GHSAT0AAAAAACGUFVMTDFSWLMIX2DUDLX7AZQ56PFA)
