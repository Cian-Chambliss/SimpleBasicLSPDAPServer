# BASIC Language Interpreter with LSP/DAP Support

A complete BASIC language interpreter written in C++ with Language Server Protocol (LSP) and Debug Adapter Protocol (DAP) support for debugging in VSCode.

## Features

### BASIC Interpreter
- **Lexer**: Tokenizes BASIC source code
- **Parser**: Recursive descent parser with AST generation
- **Runtime**: Executes BASIC programs with proper value handling
- **Variables**: Dynamic variable management
- **Functions**: Built-in functions and user-defined functions
- **Control Flow**: IF/THEN/ELSE, FOR/NEXT, WHILE/WEND, DO/LOOP
- **I/O**: PRINT and INPUT statements

### Language Server Protocol (LSP)
- **Syntax Highlighting**: Full BASIC syntax support
- **Code Completion**: Keywords, functions, and variables
- **Hover Information**: Context-sensitive help
- **Go to Definition**: Navigate to variable/function definitions
- **Find References**: Locate all usages of symbols
- **Document Symbols**: Outline view of functions and subroutines
- **Code Formatting**: Automatic code formatting
- **Error Diagnostics**: Real-time error checking

### Debug Adapter Protocol (DAP)
- **Breakpoints**: Set, remove, and manage breakpoints
- **Step Debugging**: Step into, step over, step out
- **Variable Inspection**: View and modify variables during debugging
- **Call Stack**: Navigate through function calls
- **Watch Expressions**: Monitor specific variables
- **Conditional Breakpoints**: Break on specific conditions

### VSCode Extension
- **Language Support**: Full BASIC language integration
- **Debugging**: Integrated debugging experience
- **Configuration**: Customizable interpreter settings

## Supported BASIC Syntax

### Statements
```basic
10 LET X = 10
20 PRINT "Hello, World!"
30 INPUT "Enter a number: ", Y
40 IF X > 5 THEN PRINT "X is greater than 5"
50 FOR I = 1 TO 10
60   PRINT I
70 NEXT I
80 WHILE X > 0
90   PRINT X
100   X = X - 1
110 WEND
120 END
```

### Built-in Functions
- **Math**: `ABS`, `SIN`, `COS`, `TAN`, `SQRT`, `LOG`, `EXP`
- **String**: `LEN`, `MID`, `LEFT`, `RIGHT`, `VAL`, `STR`

### User-defined Functions
```basic
FUNCTION ADD(A, B)
    ADD = A + B
END FUNCTION

LET RESULT = ADD(5, 3)
PRINT RESULT
```

## Building the Project

### Prerequisites
- CMake 3.16 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- nlohmann/json library
- Node.js and npm (for VSCode extension)

### Building the C++ Interpreter

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Install (optional)
cmake --install .
```

### Building the VSCode Extension

```bash
# Install dependencies
npm install

# Compile TypeScript
npm run compile

# Package the extension (optional)
npm run package
```

## Usage

### Running the Interpreter

```bash
# Run in interactive mode (LSP + DAP)
./basic_interpreter

# Run LSP server only
./basic_interpreter --lsp-only

# Run DAP server only
./basic_interpreter --dap-only

# Show help
./basic_interpreter --help
```

### Using with VSCode

1. **Install the Extension**:
   - Open VSCode
   - Go to Extensions (Ctrl+Shift+X)
   - Install the BASIC Language Support extension

2. **Configure the Extension**:
   - Open Settings (Ctrl+,)
   - Search for "BASIC"
   - Set the interpreter path and debug port

3. **Create a BASIC File**:
   - Create a file with `.bas` or `.basic` extension
   - Start writing BASIC code

4. **Debug a Program**:
   - Set breakpoints by clicking in the gutter
   - Press F5 to start debugging
   - Use the debug toolbar to step through code

### Example BASIC Program

Create a file named `hello.bas`:

```basic
10 PRINT "Hello, World!"
20 LET X = 10
30 FOR I = 1 TO X
40   PRINT "Count: "; I
50 NEXT I
60 INPUT "Press Enter to continue", A
70 PRINT "Goodbye!"
80 END
```

## Project Structure

```
dap/
├── CMakeLists.txt                 # Main CMake configuration
├── include/                       # Header files
│   ├── interpreter/              # BASIC interpreter headers
│   ├── lsp/                      # LSP server headers
│   └── dap/                      # DAP server headers
├── src/                          # Source files
│   ├── interpreter/              # BASIC interpreter implementation
│   ├── lsp/                      # LSP server implementation
│   ├── dap/                      # DAP server implementation
│   └── main.cpp                  # Main entry point
├── package.json                  # VSCode extension manifest
├── src/extension.ts              # VSCode extension main file
├── server/                       # LSP server TypeScript files
├── syntaxes/                     # TextMate grammar for syntax highlighting
├── language-configuration.json   # Language configuration
└── README.md                     # This file
```

## Architecture

### Interpreter Components
- **Lexer**: Converts source code to tokens
- **Parser**: Builds Abstract Syntax Tree (AST)
- **Runtime**: Executes AST nodes
- **Variables**: Manages variable storage
- **Functions**: Handles function calls and definitions

### LSP Server
- **Message Handling**: Processes LSP requests and notifications
- **Document Management**: Tracks open documents
- **Language Features**: Provides completion, hover, etc.
- **Diagnostics**: Reports errors and warnings

### DAP Server
- **Debug Session**: Manages debugging state
- **Breakpoint Management**: Handles breakpoint operations
- **Variable Inspection**: Provides variable information
- **Control Flow**: Manages step, continue, pause operations

## Configuration

### VSCode Settings

```json
{
  "basic.interpreterPath": "./basic_interpreter",
  "basic.debugPort": 4711
}
```

### Environment Variables

- `BASIC_DEBUG_PORT`: Override default debug port
- `BASIC_LOG_LEVEL`: Set logging level (DEBUG, INFO, WARN, ERROR)

## Development

### Adding New Language Features

1. **Lexer**: Add new token types in `include/interpreter/basic_interpreter.h`
2. **Parser**: Implement parsing logic in `src/interpreter/parser.cpp`
3. **Runtime**: Add execution logic in `src/interpreter/runtime.cpp`
4. **LSP**: Update completion and hover in `src/lsp/lsp_server.cpp`

### Adding New Built-in Functions

1. **Header**: Add function declaration in `include/interpreter/functions.h`
2. **Implementation**: Implement in `src/interpreter/functions.cpp`
3. **LSP**: Add to completion list in `src/lsp/lsp_server.cpp`

### Testing

```bash
# Run C++ tests
cd build
ctest

# Run VSCode extension tests
npm test
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Inspired by classic BASIC interpreters
- Built with modern C++ and TypeScript
- Uses VSCode's LSP and DAP protocols
- Leverages nlohmann/json for JSON handling

## Troubleshooting

### Common Issues

1. **Interpreter not found**: Check the `basic.interpreterPath` setting
2. **Debug port in use**: Change the `basic.debugPort` setting
3. **Syntax errors**: Check the Problems panel for detailed error messages
4. **Extension not loading**: Check the Developer Console for errors

### Debug Mode

Enable debug logging by setting the log level:

```bash
export BASIC_LOG_LEVEL=DEBUG
./basic_interpreter
```

### Getting Help

- Check the VSCode Developer Console for error messages
- Review the extension logs in the Output panel
- Open an issue on GitHub with detailed information 