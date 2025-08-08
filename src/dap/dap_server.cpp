#include "dap/dap_server.h"
#include "interpreter/runtime.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include "dap/dap_server.h"

namespace dap {

DAPServer::DAPServer()
    : running_(false), debugging_(false), paused_(false),
    currentThread_(1), currentLine_(0),
    serverSocket_(-1), clientSocket_(-1), useNetwork_(false), port_(4711),
    enableLogging_(false), stepMode_(false), nextBreakpointId_(1), checkConnection_(false) , runTillStop_(false)
{
    setupHandlers();
}

DAPServer::~DAPServer()
{

}

// Add a method to enable logging
void DAPServer::setLogging(bool enabled) {
    enableLogging_ = enabled;
}

// Modify start methods to accept logging parameter
void DAPServer::start(bool enableLogging) {
    running_ = true;
    useNetwork_ = false;
    enableLogging_ = enableLogging;
    std::cout << "Content-Type: application/vnd.microsoft.lsp-jsonrpc; charset=utf-8\r\n\r\n";
}

void DAPServer::start(int port, bool enableLogging) {
    running_ = true;
    useNetwork_ = true;
    port_ = port;
    enableLogging_ = enableLogging;
    
    // Initialize Winsock on Windows
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return;
    }
#endif
    
    // Create socket
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
#else
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
        std::cerr << "setsockopt failed" << std::endl;
        return;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return;
    }
    
    // Listen for connections
    if (listen(serverSocket_, 1) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return;
    }
    
    std::cout << "DAP server listening on port " << port_ << std::endl;
    
    // Accept client connection
    int addrlen = sizeof(address);
    clientSocket_ = accept(serverSocket_, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (clientSocket_ < 0) {
        std::cerr << "Accept failed" << std::endl;
        return;
    }
    
    std::cout << "Client connected" << std::endl;
}

void DAPServer::stop() {
    running_ = false;
    if (messageThread_.joinable()) {
        messageThread_.join();
    }
    
    // Clean up network resources
    if (useNetwork_) {
        if (clientSocket_ >= 0) {
#ifdef _WIN32
            closesocket(clientSocket_);
#else
            close(clientSocket_);
#endif
            clientSocket_ = -1;
        }
        if (serverSocket_ >= 0) {
#ifdef _WIN32
            closesocket(serverSocket_);
#else
            close(serverSocket_);
#endif
            serverSocket_ = -1;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

bool DAPServer::isRunning() const {
    return running_;
}

void DAPServer::sendMessage(const DAPMessage& message) {
    json response;
    
    if (message.type == DAPMessageType::RESPONSE) {
        response["seq"] = 1; // Simple sequence number
        response["type"] = "response";
        response["request_seq"] = message.id;
        if(!message.command.empty())
            response["command"] = message.command;
        if (!message.result.is_null()) {
            response["body"] = message.result;
        }
        if (!message.error.is_null()) {
            response["success"] = false;
            response["message"] = message.error;
        } else {
            response["success"] = true;
        }
    } else if (message.type == DAPMessageType::EVENT) {
        response["seq"] = 1;
        response["type"] = "event";
        response["event"] = message.event;
        if (!message.body.is_null()) {
            response["body"] = message.body;
        }
    }
    
    std::string content = response.dump();
    std::string header = "Content-Length: " + std::to_string(content.length()) + "\r\n\r\n";
    if (enableLogging_) {
        std::cerr << "[DAP] Sending: " << response.dump() << std::endl;
    }
    if (useNetwork_ && clientSocket_ >= 0) {
        // Send over network
        send(clientSocket_, header.c_str(), header.length(), 0);
        send(clientSocket_, content.c_str(), content.length(), 0);
    } else {
        // Send to stdout
        std::cout << header << content << std::flush;
    }
}

DAPMessage DAPServer::receiveMessage() {
    static std::string socketBuffer;

    if (runTillStop_) {
        runTillStop_ = false;
        basic::BasicInterpreter* interpreter = basic::getInterpreter();
        interpreter->continueExecution();
    }

    std::string line;
    size_t contentLength = 0;

    if (useNetwork_ && clientSocket_ >= 0) {
        // Read until we have a full header
        while (true) {
            size_t headerEnd = socketBuffer.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                // Parse headers
                std::istringstream headerStream(socketBuffer.substr(0, headerEnd));
                std::string headerLine;
                while (std::getline(headerStream, headerLine)) {
                    if (headerLine.substr(0, 16) == "Content-Length: ") {
                        contentLength = std::stoul(headerLine.substr(16));
                    }
                }
                // Remove header from buffer
                socketBuffer.erase(0, headerEnd + 4);
                break;
            }
            // Read more data from socket
            char buffer[1024];
            int bytesRead = recv(clientSocket_, buffer, sizeof(buffer), 0);
            if (bytesRead == 0)
                checkConnection_ = true;
            if (bytesRead <= 0) {
                return DAPMessage(DAPMessageType::REQUEST, "");
            }
            socketBuffer.append(buffer, bytesRead);
        }

        // Wait for the full content
        while (socketBuffer.size() < contentLength) {
            char buffer[1024];
            int bytesRead = recv(clientSocket_, buffer, sizeof(buffer), 0);
            if (bytesRead == 0)
                checkConnection_ = true;
            if (bytesRead <= 0) {
                return DAPMessage(DAPMessageType::REQUEST, "");
            }
            socketBuffer.append(buffer, bytesRead);
        }

        std::string content = socketBuffer.substr(0, contentLength);
        socketBuffer.erase(0, contentLength);

        try {
            json j = json::parse(content);
            if (enableLogging_) {
                std::cerr << "[DAP] Received: " << j.dump() << std::endl;
            }
            DAPMessage message;
            if (j["type"] == "request") {
                message.type = DAPMessageType::REQUEST;
                message.command = j["command"];
                message.id = j["seq"];
                if (j.contains("arguments")) {
                    message.arguments = j["arguments"];
                }
            } else if (j["type"] == "response") {
                message.type = DAPMessageType::RESPONSE;
                message.command = j["command"];
                message.id = j["request_seq"];
                if (j.contains("body")) {
                    message.result = j["body"];
                }
                if (j.contains("message")) {
                    message.error = j["message"];
                }
            } else if (j["type"] == "event") {
                message.type = DAPMessageType::EVENT;
                message.event = j["event"];
                if (j.contains("body")) {
                    message.body = j["body"];
                }
            }
            return message;
        } catch (const std::exception& e) {
            if (enableLogging_) {
                std::cerr << "[DAP] Receive error: " << e.what() << std::endl;
            }
            return DAPMessage(DAPMessageType::REQUEST, "");
        }
    } else {
        // Read from stdin
        std::getline(std::cin, line);
        if (line.empty()) {
            return DAPMessage(DAPMessageType::REQUEST, "");
        }
        // Parse Content-Length header
        while (!line.empty() && line != "\r") {
            if (line.substr(0, 16) == "Content-Length: ") {
                contentLength = std::stoul(line.substr(16));
            }
            std::getline(std::cin, line);
        }
        if (contentLength == 0) {
            return DAPMessage(DAPMessageType::REQUEST, "");
        }
        std::string content;
        content.resize(contentLength);
        std::cin.read(&content[0], contentLength);
        try {
            json j = json::parse(content);
            if (enableLogging_) {
                std::cerr << "[DAP] Received: " << j.dump() << std::endl;
            }
            DAPMessage message;
            if (j["type"] == "request") {
                message.type = DAPMessageType::REQUEST;
                message.command = j["command"];
                message.id = j["seq"];
                if (j.contains("arguments")) {
                    message.arguments = j["arguments"];
                }
            } else if (j["type"] == "response") {
                message.type = DAPMessageType::RESPONSE;
                message.command = j["command"];
                message.id = j["request_seq"];
                if (j.contains("body")) {
                    message.result = j["body"];
                }
                if (j.contains("message")) {
                    message.error = j["message"];
                }
            } else if (j["type"] == "event") {
                message.type = DAPMessageType::EVENT;
                message.event = j["event"];
                if (j.contains("body")) {
                    message.body = j["body"];
                }
            }
            return message;
        } catch (const std::exception& e) {
            if (enableLogging_) {
                std::cerr << "[DAP] Receive error: " << e.what() << std::endl;
            }
            return DAPMessage(DAPMessageType::REQUEST, "");
        }
    }
}

void DAPServer::processMessage(const DAPMessage& message) {
    if (message.type == DAPMessageType::REQUEST) {
        auto handler = requestHandlers_.find(message.command);
        if (handler != requestHandlers_.end()) {
            json result = handler->second(message.arguments);
            sendMessage(createResponse(message.id, result));
        }
        else if (!message.command.empty()) {
            sendMessage(createErrorResponse(message.id, -32601, "Method not found"));
        } else if(checkConnection_) {
            // Restart the socket server
            checkConnection_ = false;
            stop();
            start(port_, enableLogging_);
        }
    }
}

void DAPServer::setupHandlers() {
    // Request handlers
    requestHandlers_["initialize"] = [this](const json& args) { return handleInitialize(args); };
    requestHandlers_["launch"] = [this](const json& args) { return handleLaunch(args); };
    requestHandlers_["attach"] = [this](const json& args) { return handleAttach(args); };
    requestHandlers_["disconnect"] = [this](const json& args) { return handleDisconnect(args); };
    requestHandlers_["terminate"] = [this](const json& args) { return handleTerminate(args); };
    requestHandlers_["restart"] = [this](const json& args) { return handleRestart(args); };
    requestHandlers_["setBreakpoints"] = [this](const json& args) { return handleSetBreakpoints(args); };
    requestHandlers_["setFunctionBreakpoints"] = [this](const json& args) { return handleSetFunctionBreakpoints(args); };
    requestHandlers_["setExceptionBreakpoints"] = [this](const json& args) { return handleSetExceptionBreakpoints(args); };
    requestHandlers_["continue"] = [this](const json& args) { return handleContinue(args); };
    requestHandlers_["next"] = [this](const json& args) { return handleNext(args); };
    requestHandlers_["stepIn"] = [this](const json& args) { return handleStepIn(args); };
    requestHandlers_["stepOut"] = [this](const json& args) { return handleStepOut(args); };
    requestHandlers_["pause"] = [this](const json& args) { return handlePause(args); };
    requestHandlers_["stackTrace"] = [this](const json& args) { return handleStackTrace(args); };
    requestHandlers_["scopes"] = [this](const json& args) { return handleScopes(args); };
    requestHandlers_["variables"] = [this](const json& args) { return handleVariables(args); };
    requestHandlers_["evaluate"] = [this](const json& args) { return handleEvaluate(args); };
    requestHandlers_["setVariable"] = [this](const json& args) { return handleSetVariable(args); };
    requestHandlers_["source"] = [this](const json& args) { return handleSource(args); };
    requestHandlers_["threads"] = [this](const json& args) { return handleThreads(args); };
    requestHandlers_["modules"] = [this](const json& args) { return handleModules(args); };
    requestHandlers_["loadedSources"] = [this](const json& args) { return handleLoadedSources(args); };
    requestHandlers_["exceptionInfo"] = [this](const json& args) { return handleExceptionInfo(args); };
    requestHandlers_["loadSource"] = [this](const json& args) { return handleLoadSource(args); };
}

json DAPServer::handleInitialize(const json& arguments) {
    json capabilities = {
        {"supportsConfigurationDoneRequest", true},
        {"supportsFunctionBreakpoints", false},
        {"supportsConditionalBreakpoints", false},
        {"supportsHitConditionalBreakpoints", false},
        {"supportsEvaluateForHovers", true},
        {"exceptionBreakpointFilters", json::array()},
        {"supportsSetBreakpoints", true},
        {"supportsStepBack", false},
        {"supportsSetVariable", true},
        {"supportsRestartFrame", false},
        {"supportsGotoTargetsRequest", false},
        {"supportsStepInTargetsRequest", true},
        {"supportsCompletionsRequest", false},
        {"completionTriggerCharacters", json::array()},
        {"supportsModulesRequest", false},
        {"additionalModuleColumns", json::array()},
        {"supportedChecksumAlgorithms", json::array()},
        {"supportsRestartRequest", true},
        {"supportsExceptionOptions", false},
        {"supportsValueFormattingOptions", false},
        {"supportsExceptionInfoRequest", false},
        {"supportTerminateDebuggee", true},
        {"supportsDelayedStackTraceLoading", false},
        {"supportsLoadedSourcesRequest", true},
        {"supportsLogPoints", false},
        {"supportsTerminateThreadsRequest", false},
        {"supportsSetExpression", false},
        {"supportsTerminateRequest", true},
        {"supportsDataBreakpoints", false},
        {"supportsReadMemoryRequest", false},
        {"supportsWriteMemoryRequest", false},
        {"supportsDisassembleRequest", false},
        {"supportsBreakpointLocationsRequest", true}
    };
    return {{"capabilities", capabilities}};
}

json DAPServer::handleLaunch(const json& arguments) {
    debugging_ = true;
    currentLine_ = 0;

    std::cerr << "DAP: Launch request received" << std::endl;
    std::cerr << "DAP: Arguments: " << arguments.dump() << std::endl;

    // If "program" is a string, treat it as a filename and load its content
    if (arguments.contains("program") && arguments["program"].is_string()) {
        std::string programPath = arguments["program"];
        std::string sourceName = programPath;
        std::string content = readFileContent(programPath);
        if (!content.empty()) {
            addSource(sourceName, content);
            currentSource_ = sourceName;
        } else {
            std::cerr << "DAP: Could not read source file: " << programPath << std::endl;
        }
    } else if (arguments.contains("program") && arguments["program"].is_object()) {
        // Handle program object with path and content
        if (arguments["program"].contains("path")) {
            std::string programPath = arguments["program"]["path"];
            std::string content;
            if (arguments["program"].contains("content")) {
                content = arguments["program"]["content"];
            } else {
                content = readFileContent(programPath);
            }
            if (!content.empty()) {
                addSource(programPath, content);
                currentSource_ = programPath;
            }
        }
    } else if (arguments.contains("programPath") && arguments["programPath"].is_string()) {
        std::string programPath = arguments["programPath"];
        std::string content = readFileContent(programPath);
        if (!content.empty()) {
            addSource(programPath, content);
            currentSource_ = programPath;
        }
    }

    std::string content = getSource(currentSource_);
    if (!content.empty()) {
        // Load the program into basic interpretter
        basic::BasicInterpreter* interpreter = basic::getInterpreter();
        interpreter->loadProgram(content);
        interpreter->pause();
    }

    sendInitializedEvent();
    sendProcessEvent("BASIC Interpreter", 1);
    sendStoppedEvent("entry", currentThread_, currentLine_);

    std::cerr << "DAP: Launch completed successfully" << std::endl;

    return json::object();
}

json DAPServer::handleAttach(const json& arguments) {
    debugging_ = true;
    sendInitializedEvent();
    sendStoppedEvent("entry", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handleDisconnect(const json& arguments) {
    debugging_ = false;
    paused_ = false;
    sources_.erase(currentSource_);
    currentLine_ = 0;
    currentSource_.clear();
    basic::BasicInterpreter* interpreter = basic::getInterpreter();
    interpreter->cleanup();
    return json::object();
}

json DAPServer::handleTerminate(const json& arguments) {
    debugging_ = false;
    paused_ = false;
    sendTerminatedEvent();
    return json::object();
}

json DAPServer::handleRestart(const json& arguments) {
    // Restart debugging session
    debugging_ = false;
    paused_ = false;
    currentLine_ = 0;
    debugging_ = true;
    sendInitializedEvent();
    sendStoppedEvent("restart", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handleSetBreakpoints(const json& arguments) {
    std::string source = arguments["source"]["path"];
    json breakpoints = arguments["breakpoints"];
    json response = json::array();
    
    for (const auto& bp : breakpoints) {
        int line = bp["line"];
        setBreakpoint(source, line);
        
        json breakpoint;
        breakpoint["id"] = nextBreakpointId_++;
        breakpoint["verified"] = true;
        breakpoint["line"] = line;
        response.push_back(breakpoint);
    }
    
    return {{"breakpoints", response}};
}

json DAPServer::handleSetFunctionBreakpoints(const json& arguments) {
    return {{"breakpoints", json::array()}};
}

json DAPServer::handleSetExceptionBreakpoints(const json& arguments) {
    return json::object();
}

json DAPServer::handleContinue(const json& arguments) {
    paused_ = false;
    runTillStop_ = true;
    sendContinuedEvent(currentThread_);
    return json::object();
}

json DAPServer::handleNext(const json& arguments) {
    currentLine_++;
    basic::BasicInterpreter* interpreter = basic::getInterpreter();
    interpreter->step();
    currentLine_ = interpreter->getCurrentLine();
    sendStoppedEvent("step", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handleStepIn(const json& arguments) {
    // Simulate stepping into the next statement
    currentLine_++;
    sendStoppedEvent("step", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handleStepOut(const json& arguments) {
    currentLine_++;
    sendStoppedEvent("step", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handlePause(const json& arguments) {
    paused_ = true;
    sendStoppedEvent("pause", currentThread_, currentLine_);
    return json::object();
}

json DAPServer::handleStackTrace(const json& arguments) {
    json stackFrames = json::array();
    
    StackFrame frame;
    frame.id = 1;
    frame.name = "main";
    // Set source as an object with name, path, and sourceReference fields
    frame.source = {
        {"name", currentSource_},
        {"path", currentSource_},
        {"sourceReference", 0}
    };
    frame.line = currentLine_;
    frame.column = 0;
    
    stackFrames.push_back(frame.toJson());
    
    return {
        {"stackFrames", stackFrames},
        {"totalFrames", 1}
    };
}

json DAPServer::handleScopes(const json& arguments) {
    json scopes = json::array();
    
    Scope locals("Local");
    locals.variablesReference = 1;
    locals.namedVariables = 5;
    scopes.push_back(locals.toJson());
    
    Scope globals("Global");
    globals.variablesReference = 2;
    globals.namedVariables = 3;
    scopes.push_back(globals.toJson());
    
    return {{"scopes", scopes}};
}


json DAPServer::handleVariables(const json& arguments) {
    int variablesReference = arguments["variablesReference"];
    json variables = json::array();
    
    // Get the interpreter instance
    basic::BasicInterpreter* interpreter = basic::getInterpreter();
    if (!interpreter) {
        return {{"variables", variables}};
    }
    
    if (variablesReference == 1) {
        // Local variables - get all variables from the interpreter
        std::map<std::string, basic::Value> allVars = interpreter->getAllVariables();
        
        for (const auto& [name, value] : allVars) {
            Variable var(name);
            var.value = basic::valueToString(value);
            
            // Determine type based on the value
            std::visit([&var](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int>) var.type = "number";
                else if constexpr (std::is_same_v<T, double>) var.type = "number";
                else if constexpr (std::is_same_v<T, std::string>) var.type = "string";
                else if constexpr (std::is_same_v<T, bool>) var.type = "boolean";
                else var.type = "unknown";
            }, value);
            
            variables.push_back(var.toJson());
        }
    } else if (variablesReference == 2) {
        // Global variables - for now, same as local variables
        // In a more sophisticated implementation, you might distinguish between local and global scope
        std::map<std::string, basic::Value> allVars = interpreter->getAllVariables();
        
        for (const auto& [name, value] : allVars) {
            Variable var(name);
            var.value = basic::valueToString(value);
            
            // Determine type based on the value
            std::visit([&var](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int>) var.type = "number";
                else if constexpr (std::is_same_v<T, double>) var.type = "number";
                else if constexpr (std::is_same_v<T, std::string>) var.type = "string";
                else if constexpr (std::is_same_v<T, bool>) var.type = "boolean";
                else var.type = "unknown";
            }, value);
            
            variables.push_back(var.toJson());
        }
    }
    
    return {{"variables", variables}};
}

json DAPServer::handleEvaluate(const json& arguments) {
    std::string expression = arguments["expression"];
    json result;

    // Get the interpreter instance
    basic::BasicInterpreter* interpreter = basic::getInterpreter();
    if (!interpreter) {
        result["result"] = "[DAP] No interpreter instance available.";
        result["type"] = "error";
        result["variablesReference"] = 0;
        return result;
    }

    try {
        // Use the interpreter's public method
        auto value = interpreter->evaluateExpression(expression);
        result["result"] = basic::valueToString(value);
        // Determine type
        std::visit([&result](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>) result["type"] = "number";
            else if constexpr (std::is_same_v<T, double>) result["type"] = "number";
            else if constexpr (std::is_same_v<T, std::string>) result["type"] = "string";
            else if constexpr (std::is_same_v<T, bool>) result["type"] = "boolean";
            else result["type"] = "unknown";
        }, value);
        result["variablesReference"] = 0;
    } catch (const std::exception& e) {
        result["result"] = std::string("[DAP] Exception: ") + e.what();
        result["type"] = "error";
        result["variablesReference"] = 0;
    }
    return result;
}

json DAPServer::handleSetVariable(const json& arguments) {
    std::string name = arguments["name"];
    std::string value = arguments["value"];
    
    // In a real implementation, you'd actually set the variable
    json result;
    result["value"] = value;
    result["type"] = "string";
    result["variablesReference"] = 0;
    
    return result;
}

json DAPServer::handleSource(const json& arguments) {
    std::string path;
    if (arguments.contains("path")) {
        path = arguments["path"];
    } else if (arguments.contains("source") && arguments["source"].contains("path")) {
        path = arguments["source"]["path"];
    }
    std::string content = getSource(path); // getSource should return the content for the given path
    if (!content.empty()) {
        return {{"content", content}};
    } else {
        return {{"content", ""}};
    }
}

json DAPServer::handleThreads(const json& arguments) {
    json threads = json::array();
    
    Thread mainThread(1);
    mainThread.name = "Main Thread";
    threads.push_back(mainThread.toJson());
    
    return {{"threads", threads}};
}

json DAPServer::handleModules(const json& arguments) {
    return {{"modules", json::array()}};
}

json DAPServer::handleLoadedSources(const json& arguments) {
    json sources = json::array();
    
    for (const auto& source : getLoadedSources()) {
        sources.push_back(source.toJson());
    }
    
    return {{"sources", sources}};
}

json DAPServer::handleExceptionInfo(const json& arguments) {
    return json::object();
}

json DAPServer::handleLoadSource(const json& arguments) {
    std::string sourcePath = arguments["path"];
    std::string content = arguments["content"];
    addSource(sourcePath, content);

    // Optionally send a loadedSource event
    sendLoadedSourceEvent("new", Source(sourcePath));

    return {{"success", true}, {"path", sourcePath}};
}

// Event handlers
void DAPServer::sendInitializedEvent() {
    sendEvent("initialized", json::object());
}

void DAPServer::sendStoppedEvent(const std::string& reason, int threadId, int line) {
    json body;
    body["reason"] = reason;
    body["threadId"] = threadId;
    body["allThreadsStopped"] = true;
    body["line"] = line;
    sendEvent("stopped", body);
}

void DAPServer::sendContinuedEvent(int threadId) {
    json body;
    body["threadId"] = threadId;
    body["allThreadsContinued"] = true;
    sendEvent("continued", body);
}

void DAPServer::sendExitedEvent(int exitCode) {
    json body;
    body["exitCode"] = exitCode;
    sendEvent("exited", body);
}

void DAPServer::sendTerminatedEvent() {
    sendEvent("terminated", json::object());
}

void DAPServer::sendThreadEvent(const std::string& reason, int threadId) {
    json body;
    body["reason"] = reason;
    body["threadId"] = threadId;
    sendEvent("thread", body);
}

void DAPServer::sendOutputEvent(const std::string& category, const std::string& output) {
    json body;
    body["category"] = category;
    body["output"] = output;
    sendEvent("output", body);
}

void DAPServer::sendBreakpointEvent(const std::string& reason, const Breakpoint& breakpoint) {
    json body;
    body["reason"] = reason;
    body["breakpoint"] = breakpoint.toJson();
    sendEvent("breakpoint", body);
}

void DAPServer::sendModuleEvent(const std::string& reason, const json& module) {
    json body;
    body["reason"] = reason;
    body["module"] = module;
    sendEvent("module", body);
}

void DAPServer::sendLoadedSourceEvent(const std::string& reason, const Source& source) {
    json body;
    body["reason"] = reason;
    body["source"] = source.toJson();
    sendEvent("loadedSource", body);
}

void DAPServer::sendProcessEvent(const std::string& name, int systemProcessId) {
    json body;
    body["name"] = name;
    body["systemProcessId"] = systemProcessId;
    body["isLocalProcess"] = true;
    body["startMethod"] = "launch";
    sendEvent("process", body);
}

void DAPServer::sendCapabilitiesEvent(const json& capabilities) {
    json body;
    body["capabilities"] = capabilities;
    sendEvent("capabilities", body);
}

// Debugger control methods
void DAPServer::setBreakpoint(const std::string& source, int line) {
    breakpoints_[source].insert(line);
    
    Breakpoint bp(nextBreakpointId_++);
    bp.line = line;
    bp.source = source;
    bp.verified = true;
    breakpointMap_[bp.id] = bp;
}

void DAPServer::removeBreakpoint(const std::string& source, int line) {
    auto it = breakpoints_.find(source);
    if (it != breakpoints_.end()) {
        it->second.erase(line);
    }
}

void DAPServer::clearBreakpoints() {
    breakpoints_.clear();
    breakpointMap_.clear();
}

void DAPServer::step() {
    if (paused_) {
        currentLine_++;
        paused_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        paused_ = true;
        basic::BasicInterpreter* interpreter = basic::getInterpreter();
        interpreter->step();
        currentLine_ = interpreter->getCurrentLine();
        sendStoppedEvent("step", currentThread_, currentLine_);
    }
}

void DAPServer::stepIn() {
    currentLine_++;
    sendStoppedEvent("step", currentThread_, currentLine_);
}

void DAPServer::stepOut() {
    step();
}

void DAPServer::continueExecution() {
    paused_ = false;
    sendContinuedEvent(currentThread_);
}

void DAPServer::pause() {
    paused_ = true;
    sendStoppedEvent("pause", currentThread_, currentLine_);
}

// Debug state methods
bool DAPServer::isDebugging() const {
    return debugging_;
}

bool DAPServer::isPaused() const {
    return paused_;
}

int DAPServer::getCurrentThread() const {
    return currentThread_;
}

int DAPServer::getCurrentLine() const {
    return currentLine_;
}

std::string DAPServer::getCurrentSource() const {
    return currentSource_;
}

// Variable inspection methods
std::vector<Variable> DAPServer::getVariables(int variablesReference) {
    std::vector<Variable> variables;
    
    if (variablesReference == 1) {
        // Local variables
        Variable var1("X");
        var1.value = "10";
        var1.type = "number";
        variables.push_back(var1);
        
        Variable var2("I");
        var2.value = "5";
        var2.type = "number";
        variables.push_back(var2);
    }
    
    return variables;
}

std::vector<Scope> DAPServer::getScopes(int frameId) {
    std::vector<Scope> scopes;
    
    Scope locals("Local");
    locals.variablesReference = 1;
    locals.namedVariables = 2;
    scopes.push_back(locals);
    
    return scopes;
}

std::vector<StackFrame> DAPServer::getStackTrace(int threadId) {
    std::vector<StackFrame> frames;
    
    StackFrame frame(1);
    frame.name = "main";
    // Set source as an object with name, path, and sourceReference fields
    frame.source = {
        {"name", currentSource_},
        {"path", currentSource_},
        {"sourceReference", 0}
    };
    frame.line = currentLine_;
    frame.column = 0;
    frames.push_back(frame);
    
    return frames;
}

std::vector<Thread> DAPServer::getThreads() {
    std::vector<Thread> threads;
    
    Thread mainThread(1);
    mainThread.name = "Main Thread";
    threads.push_back(mainThread);
    
    return threads;
}

// Source management methods
void DAPServer::addSource(const std::string& path, const std::string& content) {
    std::cerr << "DAP: Adding source: " << path << " (length: " << content.length() << ")" << std::endl;
    sources_[path] = content;
    currentSource_ = path;
    std::cerr << "DAP: Total sources loaded: " << sources_.size() << std::endl;
}

std::string DAPServer::getSource(const std::string& path) const {
    auto it = sources_.find(path);
    return it != sources_.end() ? it->second : "";
}

std::vector<Source> DAPServer::getLoadedSources() {
    std::vector<Source> loadedSources;
    
    for (const auto& source : sources_) {
        Source s(source.first);
        s.path = source.first;
        loadedSources.push_back(s);
    }
    
    return loadedSources;
}

std::string DAPServer::readFileContent(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

// Helper methods
DAPMessage DAPServer::createResponse(const json& id, const json& result) {
    DAPMessage message;
    message.type = DAPMessageType::RESPONSE;
    message.id = id;
    message.result = result;
    return message;
}

DAPMessage DAPServer::createErrorResponse(const json& id, int code, const std::string& message) {
    DAPMessage dapMessage;
    dapMessage.type = DAPMessageType::RESPONSE;
    dapMessage.id = id;
    dapMessage.error = {
        {"code", code},
        {"message", message}
    };
    return dapMessage;
}

void DAPServer::sendEvent(const std::string& event, const nlohmann::json& body) {
    nlohmann::json message;
    message["type"] = "event";
    message["event"] = event;
    message["body"] = body;

    // Serialize the message to a string
    std::string content = message.dump();

    // DAP protocol requires a Content-Length header
    std::string header = "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n";

    // Write header and content to stdout (or socket if using network)
    if (useNetwork_) {
        sendNetwork(header + content);
    } else {
        std::cout << header << content << std::flush;
    }
}

int DAPServer::nextBreakpointId() {
    return nextBreakpointId_++;
}

bool DAPServer::hasBreakpoint(const std::string& source, int line) {
    auto it = breakpoints_.find(source);
    if (it != breakpoints_.end()) {
        return it->second.find(line) != it->second.end();
    }
    return false;
}

void DAPServer::updateBreakpointStatus() {
    // Update breakpoint verification status
    for (auto& pair : breakpointMap_) {
        pair.second.verified = true;
    }
}

// Called by the interpreter after each statement or line
void DAPServer::checkForStep(int line) {
    std::unique_lock<std::mutex> lock(mutex_);

    // Check if we are in step mode or if a breakpoint is set at this line
    if (shouldPauseAt(line)) {
        // Send "stopped" event to the client with the current line number
        sendStoppedEvent("step", 1, line);
        if( line > 0 ) {
            currentLine_ = line;
        }
        // Wait until the user resumes (step/continue)
        pauseCondition_.wait(lock, [this]() { return !paused_; });
    }
}

// Example helper: returns true if we should pause at this line
bool DAPServer::shouldPauseAt(int line) {
    // Check step mode or breakpoint
    if (stepMode_) return true;
    
    // Check if any breakpoint is set at this line in any source
    for (const auto& sourceBreakpoints : breakpoints_) {
        if (sourceBreakpoints.second.find(line) != sourceBreakpoints.second.end()) {
            return true;
        }
    }
    return false;
}



// Called by DAP protocol handlers when user resumes
void DAPServer::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = false;
    pauseCondition_.notify_all();
}

void DAPServer::sendNetwork(const std::string& message) {
    if (clientSocket_ >= 0) {
        send(clientSocket_, message.c_str(), message.length(), 0);
    }
}

} // namespace dap