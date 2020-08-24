# CSCE-313
All code written for my Intro to Systems course. This course was taken in Fall 2020 with Professor Sarker Tanzir Ahmed. The textbook used for this class is **[here]**(https://drive.google.com/file/d/1NrLFA0y8pYMRyVldDrwlCPZgXAcARhXm/view).

## Cloning
To clone this repository onto your local machine, run the following command in your Unix-based terminal:
```bash
git clone https://github.com/iamjeffx/CSCE-313.git
```

## Usage
Each project will have a Makefile to make compilation and executing the code much easier. For most of the Makefiles, to compile the project, run:
```bash
make all
```
To executing the project will call the command above to compile the project so you can just run the code with the following command without compiling first. Note that some of the projects will require input files; for projects with that format, check the Makefile to make sure you have the Makefile variables figured out.
```bash
make run
make run [INPUT-FILES]
```
To clean the directory of all executible and object files, run the following command:
```bash
make clean
```
