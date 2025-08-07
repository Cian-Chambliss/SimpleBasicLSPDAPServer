#include "lsp/lsp_server.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace lsp {

LSPServer::LSPServer() : running_(false) {
    setupHandlers();
}

LSPServer::~LSPServer() {
    stop();
}

void LSPServer::start() {
    running_ = true;
    std::cout << "Content-Type: application/vnd.microsoft.lsp-jsonrpc; charset=utf-8\r\n\r\n";
}

void LSPServer::stop() {
    running_ = false;
}

bool LSPServer::isRunning() const {
    return running_;
}

void LSPServer::sendMessage(const LSPMessage& message) {
    json response;
    
    if (message.type == MessageType::RESPONSE) {
        response["jsonrpc"] = "2.0";
        response["id"] = message.id;
        if (!message.result.is_null()) {
            response["result"] = message.result;
        }
        if (!message.error.is_null()) {
            response["error"] = message.error;
        }
    } else if (message.type == MessageType::NOTIFICATION) {
        response["jsonrpc"] = "2.0";
        response["method"] = message.method;
        if (!message.params.is_null()) {
            response["params"] = message.params;
        }
    }
    
    std::string content = response.dump();
    std::cout << "Content-Length: " << content.length() << "\r\n\r\n";
    std::cout << content << std::flush;
}

LSPMessage LSPServer::receiveMessage() {
    std::string line;
    std::getline(std::cin, line);
    
    if (line.empty()) {
        return LSPMessage(MessageType::UNKNOWN, "");
    }
    
    // Parse Content-Length header
    size_t contentLength = 0;
    while (!line.empty() && line != "\r") {
        if (line.substr(0, 16) == "Content-Length: ") {
            contentLength = std::stoul(line.substr(16));
        }
        std::getline(std::cin, line);
    }
    
    if (contentLength == 0) {
        return LSPMessage(MessageType::UNKNOWN, "");
    }
    
    // Read the JSON content
    std::string content;
    content.resize(contentLength);
    std::cin.read(&content[0], contentLength);
    
    try {
        json j = json::parse(content);
        LSPMessage message;
        
        if (j.contains("id")) {
            message.type = MessageType::REQUEST;
            message.id = j["id"];
            message.method = j["method"];
            if (j.contains("params")) {
                message.params = j["params"];
            }
        } else {
            message.type = MessageType::NOTIFICATION;
            message.method = j["method"];
            if (j.contains("params")) {
                message.params = j["params"];
            }
        }
        
        return message;
    } catch (const std::exception& e) {
        return LSPMessage(MessageType::UNKNOWN, "");
    }
}

void LSPServer::processMessage(const LSPMessage& message) {
    if (message.type == MessageType::REQUEST) {
        auto handler = requestHandlers_.find(message.method);
        if (handler != requestHandlers_.end()) {
            json result = handler->second(message.params);
            sendMessage(createResponse(message.id, result));
        } else {
            sendMessage(createErrorResponse(message.id, -32601, "Method not found"));
        }
    } else if (message.type == MessageType::NOTIFICATION) {
        auto handler = notificationHandlers_.find(message.method);
        if (handler != notificationHandlers_.end()) {
            handler->second(message.params);
        }
    }
}

void LSPServer::setupHandlers() {
    // Request handlers
    requestHandlers_["initialize"] = [this](const json& params) { return handleInitialize(params); };
    requestHandlers_["shutdown"] = [this](const json& params) { return handleShutdown(params); };
    requestHandlers_["textDocument/completion"] = [this](const json& params) { return handleCompletion(params); };
    requestHandlers_["textDocument/hover"] = [this](const json& params) { return handleHover(params); };
    requestHandlers_["textDocument/definition"] = [this](const json& params) { return handleDefinition(params); };
    requestHandlers_["textDocument/references"] = [this](const json& params) { return handleReferences(params); };
    requestHandlers_["textDocument/signatureHelp"] = [this](const json& params) { return handleSignatureHelp(params); };
    requestHandlers_["textDocument/documentSymbol"] = [this](const json& params) { return handleDocumentSymbol(params); };
    requestHandlers_["textDocument/formatting"] = [this](const json& params) { return handleFormatting(params); };
    requestHandlers_["workspace/symbol"] = [this](const json& params) { return handleWorkspaceSymbol(params); };
    
    // Notification handlers
    notificationHandlers_["initialized"] = [this](const json& params) { handleInitialized(params); };
    notificationHandlers_["textDocument/didOpen"] = [this](const json& params) { handleDidOpen(params); };
    notificationHandlers_["textDocument/didChange"] = [this](const json& params) { handleDidChange(params); };
    notificationHandlers_["textDocument/didClose"] = [this](const json& params) { handleDidClose(params); };
    notificationHandlers_["textDocument/didSave"] = [this](const json& params) { handleDidSave(params); };
    notificationHandlers_["workspace/didChangeConfiguration"] = [this](const json& params) { handleDidChangeConfiguration(params); };
}

json LSPServer::handleInitialize(const json& params) {
    json capabilities = {
        {"textDocumentSync", {
            {"openClose", true},
            {"change", 1}, // Incremental
            {"willSave", false},
            {"willSaveWaitUntil", false},
            {"save", {{"includeText", false}}}
        }},
        {"completionProvider", {
            {"resolveProvider", false},
            {"triggerCharacters", {".", " "}}
        }},
        {"hoverProvider", true},
        {"definitionProvider", true},
        {"referencesProvider", true},
        {"signatureHelpProvider", {
            {"triggerCharacters", {"(", ","}}
        }},
        {"documentSymbolProvider", true},
        {"documentFormattingProvider", true},
        {"workspaceSymbolProvider", true}
    };
    
    return {
        {"capabilities", capabilities},
        {"serverInfo", {
            {"name", "BASIC Language Server"},
            {"version", "1.0.0"}
        }}
    };
}

json LSPServer::handleShutdown(const json& params) {
    return json::object();
}

json LSPServer::handleCompletion(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    Position position = Position::fromJson(params["position"]);
    
    auto completions = getCompletions(uri, position);
    json items = json::array();
    
    for (const auto& completion : completions) {
        items.push_back(completion.toJson());
    }
    
    return {{"isIncomplete", false}, {"items", items}};
}

json LSPServer::handleHover(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    Position position = Position::fromJson(params["position"]);
    
    auto hover = getHover(uri, position);
    return hover.toJson();
}

json LSPServer::handleDefinition(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    Position position = Position::fromJson(params["position"]);
    
    auto definitions = getDefinitions(uri, position);
    json locations = json::array();
    
    for (const auto& def : definitions) {
        locations.push_back(def.toJson());
    }
    
    return locations;
}

json LSPServer::handleReferences(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    Position position = Position::fromJson(params["position"]);
    
    auto references = getReferences(uri, position);
    json locations = json::array();
    
    for (const auto& ref : references) {
        locations.push_back(ref.toJson());
    }
    
    return locations;
}

json LSPServer::handleSignatureHelp(const json& params) {
    return {
        {"signatures", json::array()},
        {"activeSignature", 0},
        {"activeParameter", 0}
    };
}

json LSPServer::handleDocumentSymbol(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    
    auto symbols = getDocumentSymbols(uri);
    json items = json::array();
    
    for (const auto& symbol : symbols) {
        items.push_back(symbol.toJson());
    }
    
    return items;
}

json LSPServer::handleFormatting(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    int tabSize = params.value("options", json::object()).value("tabSize", 4);
    bool insertSpaces = params.value("options", json::object()).value("insertSpaces", true);
    
    std::string content = getDocument(uri);
    if (content.empty()) {
        return json::array();
    }
    
    // Simple formatting: trim whitespace and ensure consistent indentation
    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> formattedLines;
    
    while (std::getline(iss, line)) {
        // Trim leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (!line.empty()) {
            formattedLines.push_back(line);
        }
    }
    
    json edits = json::array();
    if (!formattedLines.empty()) {
        json edit;
        edit["range"]["start"]["line"] = 0;
        edit["range"]["start"]["character"] = 0;
        edit["range"]["end"]["line"] = formattedLines.size();
        edit["range"]["end"]["character"] = 0;
        edit["newText"] = "";
        for (const auto& formattedLine : formattedLines) {
            edit["newText"] = edit["newText"].get<std::string>() + formattedLine + "\n";
        }
        edits.push_back(edit);
    }
    
    return edits;
}

json LSPServer::handleWorkspaceSymbol(const json& params) {
    std::string query = params.value("query", "");
    
    json symbols = json::array();
    
    // Search through all documents for symbols
    for (const auto& doc : documents_) {
        auto docSymbols = getDocumentSymbols(doc.first);
        for (const auto& symbol : docSymbols) {
            if (symbol.name.find(query) != std::string::npos) {
                json symbolInfo;
                symbolInfo["name"] = symbol.name;
                symbolInfo["kind"] = symbol.kind;
                symbolInfo["location"] = Location(doc.first, symbol.range).toJson();
                symbols.push_back(symbolInfo);
            }
        }
    }
    
    return symbols;
}

void LSPServer::handleInitialized(const json& params) {
    // Server is now ready to handle requests
}

void LSPServer::handleDidOpen(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    std::string content = params["textDocument"]["text"];
    addDocument(uri, content);
}

void LSPServer::handleDidChange(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    std::string content = params["contentChanges"][0]["text"];
    updateDocument(uri, content);
}

void LSPServer::handleDidClose(const json& params) {
    std::string uri = params["textDocument"]["uri"];
    removeDocument(uri);
}

void LSPServer::handleDidSave(const json& params) {
    // Document was saved
}

void LSPServer::handleDidChangeConfiguration(const json& params) {
    // Configuration changed
}

void LSPServer::addDocument(const std::string& uri, const std::string& content) {
    documents_[uri] = content;
}

void LSPServer::updateDocument(const std::string& uri, const std::string& content) {
    documents_[uri] = content;
}

void LSPServer::removeDocument(const std::string& uri) {
    documents_.erase(uri);
}

std::string LSPServer::getDocument(const std::string& uri) const {
    auto it = documents_.find(uri);
    return it != documents_.end() ? it->second : "";
}

std::vector<CompletionItem> LSPServer::getCompletions(const std::string& uri, const Position& position) {
    std::vector<CompletionItem> completions;
    
    // Add keywords
    auto keywords = getKeywords();
    for (const auto& keyword : keywords) {
        CompletionItem item(keyword);
        item.kind = "keyword";
        completions.push_back(item);
    }
    
    // Add built-in functions
    auto functions = getBuiltinFunctions();
    for (const auto& func : functions) {
        CompletionItem item(func);
        item.kind = "function";
        item.detail = "Built-in function";
        completions.push_back(item);
    }
    
    // Add built-in subroutines
    auto subroutines = getBuiltinSubroutines();
    for (const auto& sub : subroutines) {
        CompletionItem item(sub);
        item.kind = "function";
        item.detail = "Built-in subroutine";
        completions.push_back(item);
    }
    
    return completions;
}

Hover LSPServer::getHover(const std::string& uri, const Position& position) {
    std::string content = getDocument(uri);
    if (content.empty()) {
        return Hover();
    }
    
    // Simple hover implementation - could be enhanced with more sophisticated analysis
    return Hover("BASIC Language");
}

std::vector<Location> LSPServer::getDefinitions(const std::string& uri, const Position& position) {
    // Simple implementation - could be enhanced with symbol table
    return std::vector<Location>();
}

std::vector<Location> LSPServer::getReferences(const std::string& uri, const Position& position) {
    // Simple implementation - could be enhanced with symbol table
    return std::vector<Location>();
}

std::vector<DocumentSymbol> LSPServer::getDocumentSymbols(const std::string& uri) {
    std::vector<DocumentSymbol> symbols;
    std::string content = getDocument(uri);
    
    if (content.empty()) {
        return symbols;
    }
    
    std::istringstream iss(content);
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(iss, line)) {
        lineNumber++;
        
        // Look for function definitions
        if (line.find("FUNCTION ") == 0) {
            DocumentSymbol symbol;
            symbol.name = line.substr(9); // Skip "FUNCTION "
            symbol.kind = "function";
            symbol.range = Range(Position(lineNumber - 1, 0), Position(lineNumber - 1, line.length()));
            symbol.selectionRange = symbol.range;
            symbols.push_back(symbol);
        }
        
        // Look for sub definitions
        if (line.find("SUB ") == 0) {
            DocumentSymbol symbol;
            symbol.name = line.substr(4); // Skip "SUB "
            symbol.kind = "function";
            symbol.range = Range(Position(lineNumber - 1, 0), Position(lineNumber - 1, line.length()));
            symbol.selectionRange = symbol.range;
            symbols.push_back(symbol);
        }
    }
    
    return symbols;
}

std::vector<std::string> LSPServer::getKeywords() {
    return {
        "LET", "IF", "THEN", "ELSE", "FOR", "TO", "STEP", "NEXT",
        "WHILE", "WEND", "DO", "LOOP", "UNTIL", "SUB", "END",
        "FUNCTION", "RETURN", "PRINT", "INPUT", "READ", "DATA",
        "RESTORE", "DIM"
    };
}

std::vector<std::string> LSPServer::getBuiltinFunctions() {
    return {
        "ABS", "SIN", "COS", "TAN", "SQRT", "LOG", "EXP",
        "LEN", "MID", "LEFT", "RIGHT", "VAL", "STR"
    };
}

std::vector<std::string> LSPServer::getBuiltinSubroutines() {
    return {
        "PRINT", "INPUT", "READ", "DATA", "RESTORE"
    };
}

LSPMessage LSPServer::createResponse(const json& id, const json& result) {
    LSPMessage message;
    message.type = MessageType::RESPONSE;
    message.id = id;
    message.result = result;
    return message;
}

LSPMessage LSPServer::createErrorResponse(const json& id, int code, const std::string& message) {
    LSPMessage lspMessage;
    lspMessage.type = MessageType::RESPONSE;
    lspMessage.id = id;
    lspMessage.error = {
        {"code", code},
        {"message", message}
    };
    return lspMessage;
}

void LSPServer::sendNotification(const std::string& method, const json& params) {
    LSPMessage message;
    message.type = MessageType::NOTIFICATION;
    message.method = method;
    message.params = params;
    sendMessage(message);
}

} // namespace lsp 