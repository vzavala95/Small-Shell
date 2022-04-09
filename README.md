# Small-Shell
An implementation of a small shell, written in C.
Small Shell will implement a subset of features of well-known shells, such as bash. This program will:

1. Provide a prompt for running commands <br />
2. Handle blank lines and comments, which are lines beginning with the # character <br />
3. Provide expansion for the variable $$ <br />
4. Execute 3 commands exit, cd, and status via code built into the shell <br />
5. Execute other commands by creating new processes using a function from the exec family of functions <br />
6. Support input and output redirection <br />
7. Support running commands in foreground and background processes <br />
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP
