#include "cppprops.h"

#include <assert.h>
#if !__WIN64 && !__WIN32__
#include <dlfcn.h>
#else
#include "windows/dlfcn.h"
#endif
#include <stdio.h>
#include <string.h>

#include <fstream>
#include <sstream>

#include "dom.h"
#include "expression.h"
#include "interpreter.h"
#include "parser.h"
#include "utils/misc.h"

using namespace proplib;

#ifdef CORE_UTILS
#define UTILS_PATH CORE_UTILS"\\"
#else
#define UTILS_PATH ""
#endif

#define GENDIR "run/.cppprops"
#define GENSRC GENDIR "/generated.cpp"
#if !__WIN64__ && ! __WIN32__
#define GENLIB GENDIR "/" CPPPROPS_TARGET
#else
#define GENLIB GENDIR "/libcppprops.dll"
#endif

#define l(content) out << content << std::endl

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS PropertyMetadata
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
const char *CppProperties::PropertyMetadata::toString()
{
	switch( valueType )
	{
	case datalib::INT:
		sprintf( __toStringBuf, "%d", *((int *)value) );
		break;
	case datalib::FLOAT:
		sprintf( __toStringBuf, "%g", *((float *)value) );
		break;
	case datalib::BOOL:
		sprintf( __toStringBuf, "%s", *((bool *)value) ? "True" : "False" );
		break;
	default:
		assert( false );
		break;
	}

	return __toStringBuf;
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS CppProperties
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

CppProperties::LibraryUpdate CppProperties::_update = NULL;
CppProperties::LibraryGetMetadata CppProperties::_getMetadata = NULL;
Document *CppProperties::_doc = NULL;
CppProperties::UpdateContext *CppProperties::_context = NULL;

void CppProperties::init( Document *doc, UpdateContext *context )
{
	_doc = doc;
	_context = context;

	generateLibrarySource();

#if !__WIN64__ && !__WIN32__
    SYSTEM("cp " PWHOME "/etc/bld/cppprops.mak " GENDIR "/Makefile && export conf=" PWHOME "/Makefile.conf && make -C " GENDIR);
#else
    SYSTEM(UTILS_PATH "cp ./etc/bld/cppprops.win " GENDIR "/Makefile && " UTILS_PATH "cp ./etc/bld/Makefile.conf " GENDIR " && " UTILS_PATH "make -C " GENDIR);
#endif

    void *libHandle = dlopen(GENLIB, RTLD_LAZY);
    ERRIF(!libHandle, "Failed opening " GENLIB);

    typedef void (*LibraryInit) (UpdateContext *context);
    LibraryInit init = (LibraryInit) dlsym(libHandle, "__clink__CppProperties_Init");
	ERRIF( dlerror() != NULL, "%s", dlerror() );

    _update = (LibraryUpdate) dlsym(libHandle, "__clink__CppProperties_Update");
	ERRIF( dlerror() != NULL, "%s", dlerror() );

    _getMetadata = (LibraryGetMetadata) dlsym(libHandle, "__clink__CppProperties_GetMetadata");
	ERRIF( dlerror() != NULL, "%s", dlerror() );

    init(context);
}

void CppProperties::update() {
    _update(_context);
}

void CppProperties::getMetadata(PropertyMetadata **metadata, int *count) {
    _getMetadata(metadata, count);
}

void CppProperties::generateLibrarySource()
{
	CppPropertyList cppProperties;
	DynamicPropertyList dynamicProperties;
	RuntimePropertyList runtimeProperties;
	getCppProperties( _doc,
					  cppProperties,
					  dynamicProperties,
					  runtimeProperties );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		itfor( DynamicScalarAttributeMap, (*it)->attrs(), it_attr )
		{
            std::string name = it_attr->second->getName();
			if( (name != "state")
				&& (name != "init")
				&& (name != "update")
				&& (name != "end")
				&& (name != "stage") )
			{
				it_attr->second->err( "Invalid attribute name." );
			}
		}
	}


    SYSTEM(UTILS_PATH "mkdir -p " GENDIR);
    std::ofstream out(GENSRC);

	l( "// This file is machine-generated. See " << __FILE__ );
	l( "" );
#if __WIN64__ || __WIN32__
    l("");
    l("#define WIN32_LEAN_AND_MEAN");
    l("#include <windows.h>");
    l("");
#endif
    l("#include <assert.h>");
    l("#include <iostream>");
    l("#include <sstream>");
    l("#include \"agent/Metabolism.h\"");
    l("#include \"environment/barrier.h\"");
    l("#include \"genome/GenomeUtil.h\"");
    l("#include \"proplib/cppprops.h\"");
    l("#include \"proplib/state.h\"");
    l("#include \"sim/globals.h\"");
    l("#include \"sim/Simulation.h\"");
    l("#include \"utils/misc.h\"");
    l("using namespace std;");
    l("using namespace proplib;");
    l("");
    l("static bool inited = false;");

    l("");
    l("namespace proplib {");
    l("");

	// ---
	// --- Generate State Datastructures
	// ---
	generateStateStructs( out, dynamicProperties );

	// ---
	// --- Generate Metadata
	// ---
	CppPropertyInfoMap infoMap;
	generateMetadata( out, cppProperties, infoMap );

	// ---
	// --- Determine Stages
	// ---
	{
		int minStage = -1;
		for( DynamicScalarProperty *prop : dynamicProperties )
		{
			int stage = -1;
			if( prop->getAttr("stage") )
			{
				Interpreter::ExpressionEvaluator eval( prop->getAttr("stage")->getExpression() );
                std::string stageStr = eval.evaluate( prop );
				stage = atoi( stageStr.c_str() );
				if( stage < 0 )
					prop->err( "stage must be integer >= 0" );

				if( (minStage == -1) || (stage < minStage) )
					minStage = stage;
			}

			infoMap[prop].stage = stage;
		}

        l("\tstatic int stage = " << minStage << ";");
        l("");
	}

	// ---
	// --- Generate Init Function
	// ---
	generateInitSource( out, cppProperties, dynamicProperties, infoMap );

	// ---
	// --- Generate Update Function
	// ---
	generateUpdateSource( out, dynamicProperties, infoMap );

	// --- Generate C Linkage Entry Points
    l("" );
    l("// These provide public symbols we can access via dlsym()");
    l("extern \"C\" {");
    l("\tvoid __clink__CppProperties_Init(proplib::CppProperties::UpdateContext *context) {");
    l("\t\tCppProperties_Init(context);");
    l("\t}");
    l("\tvoid __clink__CppProperties_Update(proplib::CppProperties::UpdateContext *context) {");
    l("\t\tCppProperties_Update(context);" );
    l("\t}" );
    l("\tvoid __clink__CppProperties_GetMetadata(proplib::CppProperties::PropertyMetadata **result_metadata, int *result_count) {" );
    l("\t\tassert( inited );" );
    l("\t\t*result_metadata = metadata;" );
    l("\t\t*result_count = " << cppProperties.size() << ";" );
    l("\t}" );
    l("}" );

    l("" );
    l("}" );
#if __WIN64__ || __WIN32__
    l("");
    l("BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {");
    l("    switch (ul_reason_for_call) {");
    l("        case DLL_PROCESS_ATTACH:");
    l("        case DLL_THREAD_ATTACH:");
    l("        case DLL_THREAD_DETACH:");
    l("        case DLL_PROCESS_DETACH:");
    l("            break;");
    l("    }");
    l("    return TRUE;");
    l("}");
#endif
}

void CppProperties::generateStateStructs( std::ofstream &out, DynamicPropertyList &dynamicProperties )
{
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		DynamicScalarAttribute *stateAttr = prop->getAttr( "state" );
		if( stateAttr )
		{
			if( !stateAttr->getExpression()->isCppClause() )
				stateAttr->err( "'state' Should be enclosed in {}" );

			l( "struct " << getStateStructName(prop) );
			stateAttr->getExpression()->write( out );
            l( ";" << std::endl );
		}
	}
}

void CppProperties::generateMetadata( std::ofstream &out,
									  CppPropertyList &cppProperties,
									  CppPropertyInfoMap &infoMap )
{
	l( "static proplib::CppProperties::PropertyMetadata metadata[] =" );
	l( "{" );

	itfor( CppPropertyList, cppProperties, it )
	{
		__ScalarProperty *prop = *it;
		DynamicScalarProperty *dynprop = dynamic_cast<DynamicScalarProperty *>( prop );

		int index = (int)infoMap.size();
		infoMap[prop].metadataIndex = index;

		l( "  { " );

		// name
		l( "    " << '"' << prop->getFullName(1) << '"' << "," );

		// type
		if( dynprop )
			l( "    CppProperties::PropertyMetadata::Dynamic," );
		else
			l( "    CppProperties::PropertyMetadata::Runtime," );

		// valueType
		l( "    " << getDataLibType(prop) << "," );

		// value
		l( "    NULL," );

		// state
		if( dynprop && dynprop->getAttr("state") )
			l( "    new " << getStateStructName(dynprop) );
		else
			l( "    NULL" );

		if( prop != cppProperties.back() )
			l( "  }," );
		else
			l( "  }" );
	}

	l( "};" );
	l( "" );
}

void CppProperties::generateInitSource( std::ofstream &out,
										CppPropertyList &cppProperties,
										DynamicPropertyList &dynamicProperties,
										CppPropertyInfoMap &infoMap )
{
    std::map<DynamicScalarProperty *, std::string> prop2initBody;
    std::map<DynamicScalarProperty *, DynamicPropertyList> prop2antecedents;
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		prop2initBody[prop] = generateInitFunctionBody( prop, prop2antecedents[prop], infoMap );
	}

	sortDynamicProperties( dynamicProperties, prop2antecedents );

	// ---
	// --- Write Init Source
	// ---

	l( "// ------------------------------------------------------------" );
	l( "// --- CppProperties_Init()" );
	l( "// ---" );
	l( "// --- Invoked at init." );
	l( "// ------------------------------------------------------------" );
	l( "void CppProperties_Init( proplib::CppProperties::UpdateContext *context )" );
	l( "{" );
	l( "  assert( !inited );" );

	itfor( CppPropertyList, cppProperties, it )
	{
		__ScalarProperty *prop = *it;
		DynamicScalarProperty *dynprop = dynamic_cast<DynamicScalarProperty *>( prop );
		int index = infoMap[prop].metadataIndex;

		l( "  // " << prop->getFullName(1) );
		l( "  {" );
		l( "    metadata[" << index << "].value = &(" << getCppSymbol(prop) << ");" );
		if( dynprop && dynprop->getAttr("state") )
		{
			l( "    " << getStateStructName(dynprop) << " *state = (" << getStateStructName(dynprop) << "*) metadata[" << index << "].state;" );
		}

		if( dynprop && !prop2initBody[dynprop].empty() )
		{
			l( "    // START EXPRESSION" );
			l( prop2initBody[dynprop] );
			l( "    // END EXPRESSION" );
		}
		l( "  }" );
	}

	l( "  inited = true;" );
	l( "}" );
	l( "" );
}

std::string CppProperties::generateInitFunctionBody( DynamicScalarProperty *prop,
												DynamicPropertyList &antecedents,
												CppPropertyInfoMap &infoMap )
{
	if( prop->getAttr("init") == NULL )
	{
		return "";
	}
	Expression *expr = prop->getAttr( "init" )->getExpression();
    std::stringstream out;


    itfor( std::list<ExpressionElement *>, expr->elements, it )
	{
		// Skip outer '{' '}'
		if( (*it == expr->elements.front()) || (*it == expr->elements.back()) )
			continue;

		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				out << element->token->getDecorationString();
				out << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;

                std::string text;
				bool resolved = false;

				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::Property:
						if( sym.prop->getType() == Node::Scalar )
						{
							resolved = true;
							switch( sym.prop->getSubtype() )
							{
							case Node::Const:
                                text = (std::string)*sym.prop;
								// Convert Python Boolean literals to C++
								// I assume this belongs further down in the class hierarchy, but I have no idea where
								if( text == "True" )
									text = "true";
								else if( text == "False" )
									text = "false";
								break;
							case Node::Dynamic:
								antecedents.push_back( dynamic_cast<DynamicScalarProperty *>(sym.prop) );
								// fall through.
							case Node::Runtime:
								text = getMetadataLValue( sym.prop, infoMap );
								break;
							default:
								assert( false );
							}
						}
						else
						{
							if( sym.prop->getSchema()->getp("cppsym") )
							{
								resolved = true;
								text = getCppSymbol( sym.prop );
							}
							break;
						}
						break;
					case Symbol::EnumValue:
                        text = std::string("\"") + symbolPath->tail->getText() + "\"";
						break;
					default:
						// no-op
						break;
					}
				}

				// Must be a C++ symbol.
				if( !resolved )
					text = symbolPath->toString();

				out << symbolPath->head->token->getDecorationString();
				out << text;
			}
			break;
		default:
			assert( false );
		}
	}

	return out.str();
}

void CppProperties::generateUpdateSource( std::ofstream &out,
											  DynamicPropertyList &dynamicProperties,
											  CppPropertyInfoMap &infoMap )
{
    std::map<DynamicScalarProperty *, std::string> prop2updateBody;
    std::map<DynamicScalarProperty *, DynamicPropertyList> prop2antecedents;
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;

		prop2updateBody[prop] = generateUpdateFunctionBody( prop, prop2antecedents[prop], infoMap );
	}

	sortDynamicProperties( dynamicProperties, prop2antecedents );

	// ---
	// --- Write Update Source
	// ---

    std::map<int, DynamicPropertyList> stages;

	l( "// ------------------------------------------------------------" );
	l( "// --- CppProperties_Update()" );
	l( "// ---" );
	l( "// --- Invoked once per step. Updates all dynamic properties." );
	l( "// ------------------------------------------------------------" );
	l( "void CppProperties_Update( proplib::CppProperties::UpdateContext *context )" );
	l( "{" );
	l( "  assert( inited );" );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		int index = infoMap[prop].metadataIndex;

		l( "  // " << prop->getFullName(1) );
		if( infoMap[prop].stage != -1 )
		{
			l( "  if( stage == " << infoMap[prop].stage << " )" );
		}
		l( "  {" );
		l( "    struct local" );
		l( "    {" );
		l( "      static inline " << getCppType(prop) << " update( proplib::CppProperties::UpdateContext *context ) " );
		l( "      {" );
		if( prop->getAttr("state") )
		{
			l( "        " << getStateStructName(prop) << " *state = (" << getStateStructName(prop) << "*) metadata[" << index << "].state;" );
		}
		l( "        // START EXPRESSION" );
		l( prop2updateBody[prop] );
		l( "        // END EXPRESSION" );
		l( "      }" );
		l( "    };" );
		l( "    " << getCppType(prop) << " newval = local::update( context );" );
		l( "    if( newval != " << getMetadataLValue(prop, infoMap) << ")" );
		l( "    {" );
		itfor( PropertyMap, prop->getSchema()->props(), it_attr )
		{
            std::string attrName = it_attr->second->getName();

			WARN_ONCE( "Support dynamic attr asserts" );
			continue;

			if( attrName == "min" )
                l( "      assert( newval >= " << (std::string)*it_attr->second << " );" );
			else if( attrName == "exmin" )
                l( "      assert( newval > " << (std::string)*it_attr->second << " );" );
			else if( attrName == "max" )
                l( "      assert( newval <= " << (std::string)*it_attr->second << " );" );
			else if( attrName == "exmax" )
                l( "      assert( newval < " << (std::string)*it_attr->second << " );" );
		}

		l( "      *((" << getCppType(prop) << " *)metadata[" << index << "].value) = newval;" );
		l( "    }" );
		l( "  }" );
	}

	{
        std::map<int, DynamicPropertyList> stageProps;

		for( DynamicScalarProperty *prop : dynamicProperties )
			if( infoMap[prop].stage != -1 )
				stageProps[ infoMap[prop].stage ].push_back( prop );

		if( stageProps.size() > 1 )
		{
			int stages[ stageProps.size() ];
			{
				int i = 0;
                for( std::pair<int, DynamicPropertyList> stage: stageProps )
					stages[ i++ ] = stage.first;
			}

			l( "  switch( stage )" );
			l( "  {" );

			for( size_t i = 0; i < stageProps.size() - 1; i++ )
			{
				l( "    case " << stages[i] << ":" );
				for( DynamicScalarProperty *prop : stageProps[ stages[i] ] )
				{
					Interpreter::ExpressionEvaluator eval( prop->getAttr("end")->getExpression() );
                    std::string end = eval.evaluate( prop );
					l( "      if( *((" << getCppType(prop) << " *)metadata[" << infoMap[prop].metadataIndex << "].value) != " << end << ")" );
					l( "        break;" );
				}
				l( "      stage = " << stages[i + 1] << ";" );
				l( "      break;" );
			}
			l( "    case " << stages[ stageProps.size() - 1 ] << ":" );
			l( "      break; // no-op" );
			l( "    default: assert(false);" );
			l( "  }" );
		}
	}

	l( "}" );
	l( "" );
}

std::string CppProperties::generateUpdateFunctionBody( DynamicScalarProperty *prop,
												  DynamicPropertyList &antecedents,
												  CppPropertyInfoMap &infoMap )
{
	Expression *expr;
	bool skipBraces;
	if( prop->getAttr("update") != NULL )
	{
		skipBraces = true;
		expr = prop->getAttr( "update" )->getExpression();
	}
	else
	{
		skipBraces = false;
		expr = prop->getInitExpression();
	}

	bool hasReturn = false;
    std::stringstream out;

    itfor( std::list<ExpressionElement *>, expr->elements, it )
	{
		// Skip outer '{' '}'
		if( skipBraces && ( (*it == expr->elements.front()) || (*it == expr->elements.back()) ) )
			continue;

		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				out << element->token->getDecorationString();
				out << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;
                std::string symbolText = symbolPath->toString();
                std::string substitutedKeyword = "";

                std::string text;
				bool resolved = false;

				if( symbolText == "value" )
				{
					substitutedKeyword = "value";
					symbolPath->head->token->text = prop->getName();
				}
				else if( symbolText == "begin" )
				{
                    text = (std::string)*prop;
					resolved = true;
				}
				else if( symbolText == "end" )
				{
					DynamicScalarAttribute *endAttr = prop->getAttr( "end" );
					if( endAttr == NULL )
						prop->err( "No 'end' attr declaration" );

					Interpreter::ExpressionEvaluator eval( endAttr->getExpression() );

					text = eval.evaluate( prop );
					resolved = true;
				}
				else if( symbolPath->head->getText() == "return" )
				{
					hasReturn = true;
				}

				if( !resolved )
				{
					Symbol sym;

					if( prop->findSymbol(symbolPath, sym) )
					{
						switch( sym.type )
						{
						case Symbol::Property:
							if( sym.prop->getType() == Node::Scalar )
							{
								resolved = true;
								switch( sym.prop->getSubtype() )
								{
								case Node::Const:
                                    text = (std::string)*sym.prop;
									// Convert Python Boolean literals to C++
									// I assume this belongs further down in the class hierarchy, but I have no idea where
									if( text == "True" )
										text = "true";
									else if( text == "False" )
										text = "false";
									break;
								case Node::Dynamic:
									antecedents.push_back( dynamic_cast<DynamicScalarProperty *>(sym.prop) );
									// fall through.
								case Node::Runtime:
									text = getMetadataLValue( sym.prop, infoMap );
									break;
								default:
									assert( false );
								}
							}
							else
							{
								if( sym.prop->getSchema()->getp("cppsym") )
								{
									resolved = true;
									text = getCppSymbol( sym.prop );
								}
								break;
							}
							break;
						case Symbol::EnumValue:
                            text = std::string("\"") + symbolPath->tail->getText() + "\"";
							break;
						default:
							// no-op
							break;
						}
					}

					// Must be a C++ symbol.
					if( !resolved )
						text = symbolPath->toString();
				}

				// Restore original state.
				if( !substitutedKeyword.empty() )
					symbolPath->head->token->text = substitutedKeyword;

				out << symbolPath->head->token->getDecorationString();
				out << text;
			}
			break;
		default:
			assert( false );
		}
	}

	if( !hasReturn )
        return std::string("return ") + out.str() + ";";
	else
		return out.str();
}

std::string CppProperties::getStateStructName( DynamicScalarProperty *prop )
{
    return std::string("State___") + prop->getFullName( 1, "__" );
}

std::string CppProperties::getDataLibType( __ScalarProperty *prop )
{
    std::string dtype = "";

    std::string type = prop->getSchema()->get( "type" );
	if( type == "Float" )
		dtype = "FLOAT";
	if( type == "Int" )
		dtype = "INT";
	if( type == "String" )
		dtype = "STRING";
	if( type == "Bool" )
		dtype = "BOOL";

	if( dtype.empty() )
		prop->err( "No appropriate datalib type for cpp property." );

    return std::string("datalib::") + dtype;
}

std::string CppProperties::getCppType( Property *prop )
{
	Property *propType = prop->getSchema()->getp( "cpptype" );
	if( propType )
        return (std::string)*propType;
	else
	{
        std::string type = prop->getSchema()->get( "type" );
		if( type == "Float" )
			return "float";
		if( type == "Int" )
			return "int";
		if( type == "String" )
			return "string";
		if( type == "Bool" )
			return "bool";

		prop->getSchema()->err( "Cannot determine implicit cpptype" );
		return "";
	}
}

std::string CppProperties::getMetadataLValue( Property *prop_, CppPropertyInfoMap &infoMap )
{
	__ScalarProperty *prop = dynamic_cast<__ScalarProperty *>( prop_ );

    std::stringstream buf;
	buf << "*((" << getCppType(prop) << "*)metadata[/*" << prop->getFullName(1) << "*/ " << infoMap[prop].metadataIndex << "].value)";

	return buf.str();
}

static bool findSymMacro( Property &prop,
                          std::string cppsym,
						  int *result_start,
						  int *result_len,
                          std::vector<std::string> *result_args )
{
	result_args->clear();

	size_t start = cppsym.find( "$[" );
    if( start != std::string::npos )
	{
		size_t end = cppsym.find( "]", start );
        if( end == std::string::npos )
			prop.err( "Missing ']'" );

		*result_start = (int)start;
		*result_len = (int)(end - start + 1);

		size_t pos = start + 2;
		while( pos < end )
		{
			size_t tokStart = cppsym.find_first_not_of( " \t\n", pos );
			if( (cppsym[tokStart] == ',') || (cppsym[tokStart] == ']') )
			{
				pos = tokStart + 1;
				result_args->push_back( "" );
			}
			else
			{
				size_t tokEnd = cppsym.find_first_of( " \t\n,]", tokStart );
				result_args->push_back( cppsym.substr(tokStart, tokEnd - tokStart) );
				pos = cppsym.find_first_of( ",]", tokEnd ) + 1;
			}
		}

		return true;
	}

	return false;
}

std::string CppProperties::getCppSymbol( Property *prop )
{
	Property &propSym = prop->getSchema()->get( "cppsym" );
    std::string sym = propSym;

	while( true )
	{
        std::vector<std::string> macro_args;
		int macro_start;
		int macro_len;

		if( !findSymMacro(propSym, sym, &macro_start, &macro_len, &macro_args) )
			break;

		if( macro_args[0] == "sim" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[sim] takes no args." );

			sym.replace( macro_start, macro_len, "context->sim" );
		}
		else if( macro_args[0] == "index" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[index] takes no args." );
			if( prop->getParent()->getType() != Node::Array )
				propSym.err( "$[index] only valid for element of array." );

			sym.replace( macro_start, macro_len, prop->getName() );
		}
		else if( macro_args[0] == "ancestor" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[ancestor] takes no args." );

			bool resolved = false;
			for( Property *parent = prop->getParent(); parent; parent = parent->getParent() )
			{
				if( parent->getSchema()->getp("cppsym") )
				{
					resolved = true;
					sym.replace( macro_start, macro_len, getCppSymbol(parent) );
					break;
				}
			}

			if( !resolved )
				propSym.err( "Cannot resolve $[ancestor]" );
		}
		else if( macro_args[0] == "gene" )
		{
			if( macro_args.size() != 3 )
				propSym.err( "expecting $[gene, GENE_NAME, MEMBER]" );

            std::string geneName = macro_args[1];
            std::string member = macro_args[2];
            std::string expansion;

            std::string err = propSym.getLocation().getDescription() + ": Cannot find gene '" + geneName + "'";

			if( member == "min" )
			{
                expansion = std::string("genome::GeneType::to___Interpolated(genome::GenomeUtil::getGene(\"") + geneName + "\", \"" + err + "\"))->smin.__val";
			}
			else if( member == "max" )
			{
                expansion = std::string("genome::GeneType::to___Interpolated(genome::GenomeUtil::getGene(\"") + geneName + "\", \"" + err + "\"))->smax.__val";
			}
			else
			{
				propSym.err( "Invalid gene member." );
			}

			sym.replace( macro_start, macro_len, expansion );
		}
		else
		{
			break;
		}
	}

	return sym;
}

void CppProperties::getCppProperties( Property *container,
									  CppPropertyList &result_all,
									  DynamicPropertyList &result_dynamic,
									  RuntimePropertyList &result_runtime )
{
	itfor( PropertyMap, container->props(), it )
	{
		Property *prop = it->second;
		switch( prop->getType() )
		{
		case Node::Object:
		case Node::Array:
			getCppProperties( prop, result_all, result_dynamic, result_runtime );
			break;
		case Node::Scalar:
			if( prop->getSubtype() == Node::Dynamic )
			{
				result_all.push_back( dynamic_cast<__ScalarProperty *>(prop) );
				result_dynamic.push_back( dynamic_cast<DynamicScalarProperty *>(prop) );
			}
			else if( prop->getSubtype() == Node::Runtime )
			{
				result_all.push_back( dynamic_cast<__ScalarProperty *>(prop) );
				result_runtime.push_back( dynamic_cast<RuntimeScalarProperty *>(prop) );
			}
			break;
		default:
			// no-op
			break;
		}
	}
}

void CppProperties::sortDynamicProperties( DynamicPropertyList &properties,
                                           std::map<DynamicScalarProperty *, DynamicPropertyList> &prop2antecedents )
{
	struct local
	{
		static bool contains( DynamicPropertyList &result, DynamicScalarProperty *prop )
		{
			itfor( DynamicPropertyList, result, it )
				if( *it == prop )
					return true;

			return false;
		}

		static void insert( DynamicScalarProperty *prop,
							DynamicPropertyList &result,
							DynamicPropertyList &inserting,
                            std::map<DynamicScalarProperty *, DynamicPropertyList> &prop2antecedents )
		{
			if( contains(result, prop) || contains(inserting, prop) )
				return;

			inserting.push_back( prop );

			itfor( DynamicPropertyList, prop2antecedents[prop], it )
				insert( *it, result, inserting, prop2antecedents );

			result.push_back( prop );

			inserting.pop_back();
		}
	};

	DynamicPropertyList result;
	DynamicPropertyList inserting;

	itfor( DynamicPropertyList, properties, it )
		local::insert( *it, result, inserting, prop2antecedents );

	assert( result.size() == properties.size() );

	copy( result.begin(), result.end(), properties.begin() );

	assert( result.size() == properties.size() );
}
