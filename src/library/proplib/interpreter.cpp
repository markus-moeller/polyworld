#include "interpreter.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#if __WIN64__ || __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <sstream>

#include "dom.h"
#include "parser.h"
#include "utils/misc.h"
#include "utils/Resources.h"

using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ExpressionEvaluator
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Interpreter::ExpressionEvaluator::ExpressionEvaluator( Expression *expr )
: _expr( expr )
, _isEvaluating( false )
{
}

Interpreter::ExpressionEvaluator::~ExpressionEvaluator()
{
}

Expression *Interpreter::ExpressionEvaluator::getExpression()
{
	return _expr;
}

std::string Interpreter::ExpressionEvaluator::evaluate( Property *prop )
{
	if( _isEvaluating )
		prop->err( "Dependency cycle" );

	_isEvaluating = true;

	// ---
	// --- Generate Python Code
	// ---
    std::stringstream exprbuf;

    itfor( std::list<ExpressionElement *>, _expr->elements, it )
	{
		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << element->token->getDecorationString();

				// Don't add trailing semicolon
				if( (element != _expr->elements.back()) || (element->token->type != Token::Semicolon) )
					exprbuf << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;
				
				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << symbolPath->head->token->getDecorationString();
				
				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::EnumValue:
					case Symbol::Class:
						exprbuf << '"' << symbolPath->tail->getText() << '"';
						break;
					case Symbol::Property:
						{
							if( sym.prop->getType() != Node::Scalar )
                                prop->err( std::string("Illegal reference to non-scalar ") + symbolPath->toString() + "." );

							if( sym.prop->getSubtype() == Node::Runtime )
                                prop->err( std::string("Illegal reference to runtime property ") + symbolPath->toString() + ". Only dynamic expresssions may use runtime properties." );

                            std::string value = (std::string)*sym.prop;
							if( sym.prop->isEnumValue(value) || sym.prop->isString() )
								exprbuf << '"' << value << '"';
							else
								exprbuf << value;
						}
						break;
					default:
						PANIC();
					}
				}
				else
				{
					// Hopefully a Python symbol.
					exprbuf << symbolPath->toString();
				}
			}
			break;
		default:
			PANIC();
		}
	}

	// ---
	// --- Execute Python Code
	// ---
	char result[1024 * 4];

	//cout << exprbuf.str() << endl;

	bool success = Interpreter::eval( exprbuf.str(), result, sizeof(result) );
	if( !success )
	{
        prop->err( std::string("[Python] ") + result );
	}

	_isEvaluating = false;

	return result;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS InterpreterProcess
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
class InterpreterProcess {

    int const PIPE_READ = 0;
    int const PIPE_WRITE = 1;

#if !__WIN64__ && !__WIN32__

    int stdinPipe[2];
    int stdoutPipe[2];

    ssize_t write(void const *buf, size_t n) {
        return ::write(stdinPipe[PIPE_WRITE], buf, n);
    }

    ssize_t read(void *buf, size_t n) {
        return ::read(stdoutPipe[PIPE_READ], buf, n);
    }

    void createPythonProcess() {

        string script_path = Resources::getInterpreterScript();

        REQUIRE( 0 == pipe(stdinPipe) );
        REQUIRE( 0 == pipe(stdoutPipe) );

        if (0 == fork()) {
            // child process

            // redirect stdin
            REQUIRE( -1 != dup2(stdinPipe[PIPE_READ], STDIN_FILENO) );
            // redirect stdout
            REQUIRE( -1 != dup2(stdoutPipe[PIPE_WRITE], STDOUT_FILENO) );

            // run child process image
            execlp("python", "python", script_path.c_str(), NULL);
            PANIC();
        }
    }

    void closePipes() {
        close(stdinPipe[PIPE_READ]);
        close(stdinPipe[PIPE_WRITE]);
        close(stdoutPipe[PIPE_READ]);
        close(stdoutPipe[PIPE_WRITE]);
    }
#else
    HANDLE readHandle[2];
    HANDLE writeHandle[2];

    ssize_t write(void const *buf, size_t n) {
        DWORD written = 0;
        WriteFile(writeHandle[PIPE_WRITE], buf, n, &written, NULL);
        return written;
    }

    ssize_t read(void *buf, size_t n) {
        DWORD read = 0;
        ReadFile(readHandle[PIPE_READ], buf, n, &read, NULL);
        return read;
    }

    void createPythonProcess() {

        std::string script_path = Resources::getInterpreterScript();

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        REQUIRE( TRUE == CreatePipe(&readHandle[PIPE_READ], &writeHandle[PIPE_READ], &sa, 0));
        REQUIRE( TRUE == CreatePipe(&readHandle[PIPE_WRITE], &writeHandle[PIPE_WRITE], &sa, 0));

        STARTUPINFOA startInfo;
        PROCESS_INFORMATION processInfo;

        ZeroMemory(&startInfo, sizeof(startInfo));
        startInfo.cb = sizeof(startInfo);
        startInfo.hStdError = writeHandle[PIPE_READ];
        startInfo.hStdOutput = writeHandle[PIPE_READ];
        startInfo.hStdInput = readHandle[PIPE_WRITE];
        startInfo.wShowWindow = SW_SHOW;
        startInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        ZeroMemory(&processInfo, sizeof(processInfo));

//        if (NULL == GetConsoleWindow()) {
//            REQUIRE(TRUE == AllocConsole());
//        }
        char cmdLine[64];
        sprintf(cmdLine, "C:/Python27/python.exe -E %s", script_path.c_str());
        BOOL success = CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startInfo, &processInfo);
        DWORD error = GetLastError();
        REQUIRE (success == TRUE && error == 0);
    }

    void closePipes() {
        CloseHandle(readHandle[PIPE_READ]);
        CloseHandle(readHandle[PIPE_WRITE]);
        CloseHandle(writeHandle[PIPE_READ]);
        CloseHandle(writeHandle[PIPE_WRITE]);
//        FreeConsole();
    }

#endif

public:
    InterpreterProcess() {
        createPythonProcess();
    }

    ~InterpreterProcess() {
        // for Python 2.x
        write("exit\n", 5);

        // for Python 3.x
        //write("exit()\n", 7);
        closePipes();
    }

    bool eval(const std::string &expr, char *result, size_t result_size) {
        
        char writebuf[1024*8];
        sprintf(writebuf, "<expr>\n%s\n</expr>\n", expr.c_str());
        size_t writelen = strlen(writebuf);
        REQUIRE( ssize_t(writelen) == write(writebuf, writelen) );

        char success;
        REQUIRE( 1 == read(&success, 1) );
        REQUIRE( success == 'S' || success == 'F' );
        
        char readlen_str[11];
        REQUIRE( 10 == read(readlen_str, 10) );
        readlen_str[10] = '\0';
        size_t readlen = (size_t)atoi(readlen_str);
        
        REQUIRE( readlen < result_size );
        REQUIRE( ssize_t(readlen) == read(result, readlen) );
        result[readlen] = '\0';

        return success == 'S';
    }
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Interpreter
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

InterpreterProcess *Interpreter::process = nullptr;

void Interpreter::init()
{
	REQUIRE( !process );
    process = new InterpreterProcess();
}

void Interpreter::dispose()
{
	REQUIRE( process );
    delete process;
    process = nullptr;
}

bool Interpreter::eval(const std::string &expr,	char *result, size_t result_size) {
    return process->eval(expr, result, result_size);
}
