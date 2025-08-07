#include <iostream>
#include <string>
#include <thread>
#include <signal.h>
#include <memory>

#include "lsp/lsp_server.h"
#include "dap/dap_server.h"
#include "interpreter/basic_interpreter.h"

using namespace lsp;
using namespace dap;
using namespace basic;
namespace basic {
void setInterpreter(BasicInterpreter* interpreter);
}

std::unique_ptr<LSPServer> lspServer;
std::unique_ptr<DAPServer> dapServer;
std::unique_ptr<BasicInterpreter> interpreter;
bool running = true;

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
    
    if (lspServer) lspServer->stop();
    if (dapServer) dapServer->stop();
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "Options:\n"
              << "  --lsp-only     Run only the Language Server Protocol server\n"
              << "  --dap-only     Run only the Debug Adapter Protocol server\n"
              << "  --interactive  Run in interactive mode (default)\n"
              << "  --port <port>  Specify the port for the DAP server (default: 4711)\n"
              << "  --log-dap      Enable logging for the Debug Adapter Protocol server\n"
              << "  --help         Show this help message\n"
              << "\n"
              << "When running in interactive mode, the server will:\n"
              << "1. Start LSP server on stdin/stdout for language features\n"
              << "2. Start DAP server on a separate port for debugging\n"
              << "3. Initialize the BASIC interpreter\n"
              << "\n"
              << "Example BASIC program:\n"
              << "10 PRINT \"Hello, World!\"\n"
              << "20 LET X = 10\n"
              << "30 FOR I = 1 TO X\n"
              << "40   PRINT I\n"
              << "50 NEXT I\n"
              << "60 END\n";
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Parse command line arguments
    bool lspOnly = false;
    bool dapOnly = false;
    bool interactive = true;
    int port = 4711;
    bool enableLogging = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lsp-only") {
            lspOnly = true;
            interactive = false;
        } else if (arg == "--dap-only") {
            dapOnly = true;
            interactive = false;
        } else if (arg == "--interactive") {
            interactive = true;
        } else if (arg == "--log-dap") {
            enableLogging = true;
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    try {
        // Initialize the BASIC interpreter
        interpreter = std::make_unique<BasicInterpreter>();
        
        if (interactive || lspOnly) {
            std::cout << "Starting BASIC Language Server..." << std::endl;
            lspServer = std::make_unique<LSPServer>();
            lspServer->start();
        }
        
        if (interactive || dapOnly) {
            std::cout << "Starting BASIC Debug Adapter..." << std::endl;
            dapServer = std::make_unique<DAPServer>();
            if (dapOnly) {
                dapServer->start(4711, enableLogging);  // Use network mode for DAP-only
                basic::setDAPServer(dapServer.get());
                basic::setInterpreter(interpreter.get());
            } else {
                dapServer->start(enableLogging);  // Use stdin/stdout for interactive mode
                basic::setDAPServer(dapServer.get());
                basic::setInterpreter(interpreter.get());
            }
            
        }
        
        if (interactive) {
            std::cout << "BASIC Interpreter with LSP/DAP support is running." << std::endl;
            std::cout << "LSP server: stdin/stdout" << std::endl;
            std::cout << "DAP server: port 4711" << std::endl;
            std::cout << "Press Ctrl+C to exit." << std::endl;
            
            // Main event loop
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Process LSP messages
                if (lspServer && lspServer->isRunning()) {
                    try {
                        LSPMessage message = lspServer->receiveMessage();
                        if (message.type != MessageType::NOTIFICATION) {
                            lspServer->processMessage(message);
                        }
                    } catch (const std::exception& e) {
                        // Handle LSP communication errors
                    }
                }
                
                // Process DAP messages
                if (dapServer && dapServer->isRunning()) {
                    try {
                        DAPMessage message = dapServer->receiveMessage();
                        if (message.type != DAPMessageType::EVENT) {
                            dapServer->processMessage(message);
                        }
                    } catch (const std::exception& e) {
                        // Handle DAP communication errors
                    }
                }
            }
        } else if (lspOnly) {
            // LSP-only mode
            std::cout << "LSP server running on stdin/stdout" << std::endl;
            while (running && lspServer->isRunning()) {
                try {
                    LSPMessage message = lspServer->receiveMessage();
                    if (message.type != MessageType::NOTIFICATION) {
                        lspServer->processMessage(message);
                    }
                } catch (const std::exception& e) {
                    break;
                }
            }
        } else if (dapOnly) {
            // DAP-only mode
            std::cout << "DAP server running on port 4711" << std::endl;
            while (running && dapServer->isRunning()) {
                try {
                    DAPMessage message = dapServer->receiveMessage();
                    if (message.type != DAPMessageType::EVENT) {
                        dapServer->processMessage(message);
                    }
                } catch (const std::exception& e) {
                    break;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Shutting down BASIC Interpreter..." << std::endl;
    
    if (lspServer) {
        lspServer->stop();
    }
    
    if (dapServer) {
        dapServer->stop();
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
} 