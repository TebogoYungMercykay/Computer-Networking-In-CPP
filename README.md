# Computer Networking: Network Protocols Implementation

<img src="images/readme.jpg" style="width: 100%; height: 40%;" />

A comprehensive collection of network protocol implementations and server applications built from scratch to demonstrate core networking concepts and server-side programming.

## Overview

This repository contains implementations of various network protocols and server applications, each built to understand the fundamental mechanics of network communication without relying on high-level libraries or frameworks.

## Components

**cgi-scripts/** - Common Gateway Interface programs for server-side execution and dynamic web content generation

- Time zone display applications
- Dynamic content generation
- Backend file manipulation

**ftp-server/** - File Transfer Protocol server implementation

- Control and data connection handling
- File upload/download capabilities
- Multi-client support

**http-server/** - HTTP server supporting static file serving, dynamic content generation, and form processing

- Custom routing mechanisms
- Calculator web application interface

**ldap-389/** - Lightweight Directory Access Protocol client implementation

- Directory Information Tree queries
- Binary message encoding/decoding
- Authentication and search operations

**pop3-server/** - Post Office Protocol 3 server for email retrieval

- Email message management
- User authentication
- Message deletion and retrieval

**smtp-server/** - Simple Mail Transfer Protocol server implementation

- Email sending capabilities
- SMTP command processing
- Message queuing and delivery

**telnet/** - Telnet server implementation

- Remote terminal access
- ANSI escape sequence support
- Multi-user capabilities

## Technical Implementation

### Core Principles

- Socket-level programming using raw sockets
- Protocol compliance with RFC specifications
- No high-level networking libraries
- Server-side focus with multi-threaded architectures

### Features

- Binary and text protocol handling
- Custom message parsing and generation
- Cross-platform compatibility
- Comprehensive error handling

## Prerequisites

- Linux/Unix environment
- GCC compiler
- Apache web server (for CGI components)
- OpenLDAP server (for LDAP components)

## Installation

1. Clone the repository
2. Navigate to individual project directories
3. Compile using provided Makefiles
4. Configure server settings as needed
5. Deploy to appropriate server directories

Each component includes specific configuration files and setup instructions.

## Protocol Coverage

- HTTP/1.1 with CGI support
- FTP with active/passive modes
- SMTP for email transmission
- POP3 for email retrieval
- LDAP for directory services
- Telnet for remote terminal access

## Security Notice

These implementations are designed for educational purposes and protocol understanding. They may not include production-level security features.

## Credits

- [NetworkProf](https://youtube.com/playlist?list=PLtjT6PTtgrGZCJMMQdti2AQa85P8LQIWy&feature=shared)
- [Engineering Funda](https://www.youtube.com/watch?v=hOEj_0GFh2g&list=PLgwJf8NK-2e5utf4e5VJCEeNTDFtKHgsF)
- [Feduguide](https://www.google.com/url?sa=i&url=https%3A%2F%2Feduguide.co.in%2Fwhat-are-the-4-main-types-of-computer-networks%2F&psig=AOvVaw2OKIWpp_98g_WIgo4xgEGZ&ust=1749083861589000&source=images&cd=vfe&opi=89978449&ved=0CBcQjhxqFwoTCNDdlPzD1o0DFQAAAAAdAAAAABAE)

Feel free to explore the notes and practicals to enhance your understanding of the OSI model and its applications in real-world networking scenarios. For any questions or further clarifications, please refer to the contact section in the main repository README file.

---

---
