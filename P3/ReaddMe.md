# Micro shell (ush). 

ush is the micro (mu) shell command interpreter with a syntax similar to UNIX C shell.

### Initialization and Termination
When first stared, ush normally performs commands from the file ˜/.ushrc, provided that it is readable. Commands in this file are processed just the same as if they were taken from standard input.

### Interactive Operation
After startup processing, an interactive ush shell begins reading commands from the terminal, prompting with hostname%. The shell then repeatedly performs the following actions: a line of command input is read and broken into words; this sequence of words is parsed (as described under Usage); and the shell executes each command in the current line.

This Micro shell (ush) has following built-in commands:
- setenv
- unsetenv
- cd
- echo
- where
- nice
- pwd
- logout


This also supports I/O redirection and multi pipe commands.

More information can be found in the attached pdf file.

