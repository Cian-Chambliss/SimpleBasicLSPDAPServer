#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

namespace lsp {

using json = nlohmann::json;

// LSP message types
enum class MessageType {
    UNKNOWN,
    REQUEST,
    RESPONSE,
    NOTIFICATION
};

// LSP request types
enum class RequestType {
    INITIALIZE,
    SHUTDOWN,
    TEXTDOCUMENT_COMPLETION,
    TEXTDOCUMENT_HOVER,
    TEXTDOCUMENT_DEFINITION,
    TEXTDOCUMENT_REFERENCES,
    TEXTDOCUMENT_SIGNATUREHELP,
    TEXTDOCUMENT_DOCUMENTSYMBOL,
    TEXTDOCUMENT_FORMATTING,
    WORKSPACE_SYMBOL
};

// LSP notification types
enum class NotificationType {
    INITIALIZED,
    TEXTDOCUMENT_DIDOPEN,
    TEXTDOCUMENT_DIDCHANGE,
    TEXTDOCUMENT_DIDCLOSE,
    TEXTDOCUMENT_DIDSAVE,
    WORKSPACE_DIDCHANGECONFIGURATION
};

// LSP message structure
struct LSPMessage {
    MessageType type;
    std::string method;
    json params;
    json id;
    json result;
    json error;
    
    LSPMessage() = default;
    LSPMessage(MessageType t, const std::string& m) : type(t), method(m) {}
};

// Position in document
struct Position {
    int line;
    int character;
    
    Position(int l = 0, int c = 0) : line(l), character(c) {}
    
    json toJson() const {
        return json{{"line", line}, {"character", character}};
    }
    
    static Position fromJson(const json& j) {
        return Position(j["line"], j["character"]);
    }
};

// Range in document
struct Range {
    Position start;
    Position end;
    
    Range(const Position& s = Position(), const Position& e = Position()) 
        : start(s), end(e) {}
    
    json toJson() const {
        return json{{"start", start.toJson()}, {"end", end.toJson()}};
    }
    
    static Range fromJson(const json& j) {
        return Range(Position::fromJson(j["start"]), Position::fromJson(j["end"]));
    }
};

// Location in document
struct Location {
    std::string uri;
    Range range;
    
    Location(const std::string& u = "", const Range& r = Range()) : uri(u), range(r) {}
    
    json toJson() const {
        return json{{"uri", uri}, {"range", range.toJson()}};
    }
};

// Completion item
struct CompletionItem {
    std::string label;
    std::string detail;
    std::string documentation;
    std::string kind;
    
    CompletionItem(const std::string& l = "") : label(l) {}
    
    json toJson() const {
        json j = {{"label", label}};
        if (!detail.empty()) j["detail"] = detail;
        if (!documentation.empty()) j["documentation"] = documentation;
        if (!kind.empty()) j["kind"] = kind;
        return j;
    }
};

// Hover information
struct Hover {
    std::string contents;
    Range range;
    
    Hover(const std::string& c = "") : contents(c) {}
    
    json toJson() const {
        return json{{"contents", contents}, {"range", range.toJson()}};
    }
};

// Document symbol
struct DocumentSymbol {
    std::string name;
    std::string detail;
    std::string kind;
    Range range;
    Range selectionRange;
    std::vector<DocumentSymbol> children;
    
    DocumentSymbol(const std::string& n = "") : name(n) {}
    
    json toJson() const {
        json j = {
            {"name", name},
            {"kind", kind},
            {"range", range.toJson()},
            {"selectionRange", selectionRange.toJson()}
        };
        if (!detail.empty()) j["detail"] = detail;
        if (!children.empty()) j["children"] = json::array();
        for (const auto& child : children) {
            j["children"].push_back(child.toJson());
        }
        return j;
    }
};

// LSP Server class
class LSPServer {
public:
    LSPServer();
    ~LSPServer();
    
    // Main server methods
    void start();
    void stop();
    bool isRunning() const;
    
    // Message handling
    void sendMessage(const LSPMessage& message);
    LSPMessage receiveMessage();
    void processMessage(const LSPMessage& message);
    
    // Request handlers
    json handleInitialize(const json& params);
    json handleShutdown(const json& params);
    json handleCompletion(const json& params);
    json handleHover(const json& params);
    json handleDefinition(const json& params);
    json handleReferences(const json& params);
    json handleSignatureHelp(const json& params);
    json handleDocumentSymbol(const json& params);
    json handleFormatting(const json& params);
    json handleWorkspaceSymbol(const json& params);
    
    // Notification handlers
    void handleInitialized(const json& params);
    void handleDidOpen(const json& params);
    void handleDidChange(const json& params);
    void handleDidClose(const json& params);
    void handleDidSave(const json& params);
    void handleDidChangeConfiguration(const json& params);
    
    // Document management
    void addDocument(const std::string& uri, const std::string& content);
    void updateDocument(const std::string& uri, const std::string& content);
    void removeDocument(const std::string& uri);
    std::string getDocument(const std::string& uri) const;
    
    // Language features
    std::vector<CompletionItem> getCompletions(const std::string& uri, const Position& position);
    Hover getHover(const std::string& uri, const Position& position);
    std::vector<Location> getDefinitions(const std::string& uri, const Position& position);
    std::vector<Location> getReferences(const std::string& uri, const Position& position);
    std::vector<DocumentSymbol> getDocumentSymbols(const std::string& uri);

private:
    bool running_;
    std::map<std::string, std::string> documents_;
    std::map<std::string, std::function<json(const json&)>> requestHandlers_;
    std::map<std::string, std::function<void(const json&)>> notificationHandlers_;
    
    void setupHandlers();
    LSPMessage createResponse(const json& id, const json& result);
    LSPMessage createErrorResponse(const json& id, int code, const std::string& message);
    void sendNotification(const std::string& method, const json& params);
    
    // Helper methods for language features
    std::vector<std::string> getKeywords();
    std::vector<std::string> getBuiltinFunctions();
    std::vector<std::string> getBuiltinSubroutines();
};

} // namespace lsp 