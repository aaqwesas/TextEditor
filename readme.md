# Simple Text Editor in C

## Introduction

This project is a simple text editor implemented in C. It allows users to create, edit, and save text files directly from the console. The text editor is designed to be lightweight and easy to use, providing a basic set of functionalities that mimic common text editing features.

it is a project that based on the design of [kilo](https://viewsourcecode.org/snaptoken/kilo/index.html)

## Features

- Create a new text file
- Open and edit existing text files
- Save changes to files
- Basic navigation using arrow keys
- Simple user interface with menu options
- Supports multi-line text input

## Installation

To get started with the text editor, follow these steps:

1. **Clone the repository**:

```

git clone https://github.com/aaqwesas/TextEditor.git
cd TextEditor

```

2. **Compile the program**:
   use makefile to compile the program

```
make

```

3. **Run the program**:
   After compiling, you can run the text editor with:

```

./kilo
```

or

```
./kilo filename
```

## Usage

1. **Create New File**: Start the program without any arguments to create a new file.
2. **Open Existing File**: Provide a filename as an argument to open an existing file.
3. **Editing Text**: Type your text directly into the editor. Use arrow keys for navigation.
4. **Saving Changes**: When finished editing, click CTRL + S to save.
5. **Exit Editor**: Click CTRL + Q to quit the program.
6. When entered the save mode, click ESC to exit the save mode and go back to edit mode.
