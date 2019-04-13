# bits-counter
OS and System Programming lab #3.  
Linux-based OS required.  
Contains c-language program and script to control results(for one file)  
Task: Count number of set bits in each file from directory and all subdirectories.  
The main process opens directories and starts a separate bit-counting process for each file.  
Each process displays its own pid, the full path to the file, the total number of bytes scanned and the number of bits 0 and 1.  
The number of simultaneously running processes must not exceed N (entered by the user).  
# Run: 
```
gcc -o runnable main.c  
./runnable <start_dir> <output_file> <max_processes>  
```
For script:
```
chmod +x script.h  
./script.h <file>  
```
