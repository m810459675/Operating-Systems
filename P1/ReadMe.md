# Fend -- Simple Sandbox

This program (Fend) restricts the child program's access to various files and directories in the file system based on the permission given in the input config file.

If no config file is inputed for the program, it checks the .fendrc file in the running directory first if not present then it checks the user's home path. If the config file is not present anywhere it throws an error message and exits.

### Input Config file format:

Permission File_Name

Permission is the octal 3 digit number similar to the ones used in chmod.

Example:

777 specifies all access to the given file

### Command for execution:

./fend {Child_Program} [-f Config_File]
