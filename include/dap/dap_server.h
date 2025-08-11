#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace dap {

using json = nlohmann::json;

// DAP message types
enum class DAPMessageType {
    REQUEST,
    RESPONSE,
    EVENT
};

// DAP request types
enum class DAPRequestType {
    INITIALIZE,
    LAUNCH,
    ATTACH,
    DISCONNECT,
    TERMINATE,
    RESTART,
    SETBREAKPOINTS,
    SETFUNCTIONBREAKPOINTS,
    SETEXCEPTIONBREAKPOINTS,
    CONTINUE,
    NEXT,
    STEPIN,
    STEPOUT,
    PAUSE,
    STACKTRACE,
    SCOPES,
    VARIABLES,
    EVALUATE,
    SETVARIABLE,
    SOURCE,
    THREADS,
    MODULES,
    LOADEDSOURCES,
    EXCEPTIONINFO,
    READMEMORY,
    WRITEMEMORY,
    DISASSEMBLE
};

// DAP event types
enum class DAPEventType {
    INITIALIZED,
    STOPPED,
    CONTINUED,
    EXITED,
    TERMINATED,
    THREAD,
    OUTPUT,
    BREAKPOINT,
    MODULE,
    LOADEDSOURCE,
    PROCESS,
    CAPABILITIES
};

// DAP message structure
struct DAPMessage {
    DAPMessageType type;
    std::string command;
    json arguments;
    json id;
    json result;
    json error;
    std::string event;
    json body;
    
    DAPMessage() = default;
    DAPMessage(DAPMessageType t, const std::string& cmd) : type(t), command(cmd) {}
};

// Breakpoint information
struct Breakpoint {
    int id;
    bool verified;
    std::string message;
    int line;
    int column;
    std::string source;
    
    Breakpoint(int i = 0) : id(i), verified(false), line(0), column(0) {}
    
    json toJson() const {
        json j = {
            {"id", id},
            {"verified", verified},
            {"line", line}
        };
        if (!message.empty()) j["message"] = message;
        if (!source.empty()) j["source"] = source;
        if (column > 0) j["column"] = column;
        return j;
    }
};

// Stack frame information
struct StackFrame {
    int id;
    std::string name;
    json source;
    int line;
    int column;
    int endLine;
    int endColumn;
    
    StackFrame(int i = 0) : id(i), line(0), column(0), endLine(0), endColumn(0) {}
    
    json toJson() const {
        json j = {
            {"id", id},
            {"name", name},
            {"line", line},
            {"column", column}
        };
        if (!source.empty()) j["source"] = source;
        if (endLine > 0) j["endLine"] = endLine;
        if (endColumn > 0) j["endColumn"] = endColumn;
        return j;
    }
};

// Variable information
struct Variable {
    std::string name;
    std::string value;
    std::string type;
    std::string kind;
    int variablesReference;
    int indexedVariables;
    int namedVariables;
    
    Variable(const std::string& n = "") : name(n), variablesReference(0), indexedVariables(0), namedVariables(0) {}
    
    json toJson() const {
        json j = {
            {"name", name},
            {"value", value},
            {"variablesReference", variablesReference}
        };
        if (!type.empty()) j["type"] = type;
        if (!kind.empty()) j["kind"] = kind;
        if (indexedVariables > 0) j["indexedVariables"] = indexedVariables;
        if (namedVariables > 0) j["namedVariables"] = namedVariables;
        return j;
    }
};

// Scope information
struct Scope {
    std::string name;
    std::string presentationHint;
    int variablesReference;
    int namedVariables;
    int indexedVariables;
    bool expensive;
    
    Scope(const std::string& n = "") : name(n), variablesReference(0), namedVariables(0), indexedVariables(0), expensive(false) {}
    
    json toJson() const {
        return json{
            {"name", name},
            {"presentationHint", presentationHint},
            {"variablesReference", variablesReference},
            {"namedVariables", namedVariables},
            {"indexedVariables", indexedVariables},
            {"expensive", expensive}
        };
    }
};

// Thread information
struct Thread {
    int id;
    std::string name;
    
    Thread(int i = 0) : id(i) {}
    
    json toJson() const {
        return json{{"id", id}, {"name", name}};
    }
};

// Source information
struct Source {
    std::string name;
    std::string path;
    int sourceReference;
    
    Source(const std::string& n = "") : name(n), sourceReference(0) {}
    
    json toJson() const {
        json j = {{"name", name}};
        if (!path.empty()) j["path"] = path;
        if (sourceReference > 0) j["sourceReference"] = sourceReference;
        return j;
    }
};

// DAP Server class
class DAPServer {
public:
    DAPServer();
    ~DAPServer();
    void setLogging(bool enabled);

    // Main server methods
    void start(bool enableLogging);  // Start with stdin/stdout communication
    void start(int port, bool enableLogging);  // Start with network support on specified port
    void stop();
    bool isRunning() const;
    
    // Message handling
    void sendMessage(const DAPMessage& message);
    DAPMessage receiveMessage();
    void processMessage(const DAPMessage& message);
    
    // Request handlers
    json handleInitialize(const json& arguments);
    json handleLaunch(const json& arguments);
    json handleAttach(const json& arguments);
    json handleDisconnect(const json& arguments);
    json handleTerminate(const json& arguments);
    json handleRestart(const json& arguments);
    json handleSetBreakpoints(const json& arguments);
    json handleSetFunctionBreakpoints(const json& arguments);
    json handleSetExceptionBreakpoints(const json& arguments);
    json handleContinue(const json& arguments);
    json handleNext(const json& arguments);
    json handleStepIn(const json& arguments);
    json handleStepOut(const json& arguments);
    json handlePause(const json& arguments);
    json handleStackTrace(const json& arguments);
    json handleScopes(const json& arguments);
    json handleVariables(const json& arguments);
    json handleEvaluate(const json& arguments);
    json handleSetVariable(const json& arguments);
    json handleSource(const json& arguments);
    json handleThreads(const json& arguments);
    json handleModules(const json& arguments);
    json handleLoadedSources(const json& arguments);
    json handleExceptionInfo(const json& arguments);
    json handleLoadSource(const json &arguments);
    json handleReadMemory(const json &arguments);
    json handleWriteMemory(const json& arguments);
    json handleDisassemble(const json& arguments);
    json handleConfigurationDone(const json& arguments);
    
    void NestedEventHandler();

    // Event handlers
    void sendInitializedEvent();
    void sendStoppedEvent(const std::string& reason, int threadId = 1, int line = 0);
    void sendContinuedEvent(int threadId = 1);
    void sendExitedEvent(int exitCode = 0);
    void sendTerminatedEvent();
    void sendThreadEvent(const std::string& reason, int threadId = 1);
    void sendOutputEvent(const std::string& category, const std::string& output);
    void sendBreakpointEvent(const std::string& reason, const Breakpoint& breakpoint);
    void sendModuleEvent(const std::string& reason, const json& module);
    void sendLoadedSourceEvent(const std::string& reason, const Source& source);
    void sendProcessEvent(const std::string& name, int systemProcessId);
    void sendCapabilitiesEvent(const json& capabilities);
    
    // Debug stepping support
    void checkForStep(int line);
    
    // Debugger control
    void setBreakpoint(const std::string& source, int line);
    void removeBreakpoint(const std::string& source, int line);
    void clearBreakpoints();
    void step();
    void stepIn();
    void stepOut();
    void continueExecution();
    void pause();
    
    // Debug state
    bool isDebugging() const;
    bool isPaused() const;
    int getCurrentThread() const;
    int getCurrentLine() const;
    std::string getCurrentSource() const;
    
    // Variable inspection
    std::vector<Variable> getVariables(int variablesReference);
    std::vector<Scope> getScopes(int frameId);
    std::vector<StackFrame> getStackTrace(int threadId);
    std::vector<Thread> getThreads();
    
    // Source management
    void addSource(const std::string& path, const std::string& content);
    std::string getSource(const std::string& path) const;
    std::vector<Source> getLoadedSources();
    std::string readFileContent(const std::string& path) const;

private:
    bool running_;
    bool debugging_;
    bool paused_;
    int currentThread_;
    int currentLine_;
    std::string currentSource_;
    
    // Network support
    int serverSocket_;
    int clientSocket_;
    bool useNetwork_;
    int port_;
    bool checkConnection_;

    bool enableLogging_;

    // Debugger state
    bool stepMode_ = false;
    bool runTillStop_ = false;

    // Breakpoints: map from source file to set of line numbers
    std::map<std::string, std::set<int>> breakpoints_;
    std::map<int, Breakpoint> breakpointMap_;
    int nextBreakpointId_;

    // Source management
    std::map<std::string, std::string> sources_;

    // Synchronization for stepping and pausing
    std::mutex mutex_;
    std::condition_variable pauseCondition_;

    std::map<std::string, std::function<json(const json&)>> requestHandlers_;
    
    // Threading
    std::thread messageThread_;
    
    void setupHandlers();
    DAPMessage createResponse(const json& id, const json& result);
    DAPMessage createErrorResponse(const json& id, int code, const std::string& message);
    void sendEvent(const std::string& event, const json& body);
    
    // Helper methods
    int nextBreakpointId();
    bool hasBreakpoint(const std::string& source, int line);
    void updateBreakpointStatus();
    bool shouldPauseAt(int line);
    void resume();
    void sendNetwork(const std::string& message);
};

} // namespace dap 

namespace basic {
    void setDAPServer(dap::DAPServer* server);
}