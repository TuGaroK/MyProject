#include <iostream>
#include <vector>
#include <FlexLexer.h>
#include "lex.yy.c"
#include <unordered_map>

/*
some defenicion in Verilog

T = {module, endmodule, input, output, wire, not,and, nand, end, or, xor, /n}
N = { =, (, )}

stmt ->  module* EOF
module -> MODULE name "(" name(,name)*")"; decl* expr* | ENDMODULE
decl -> (input|output|wire)?[NUM:NUM]? name(,name)* ;// тут мы будем проверять, есть ли далее скобка, так как она опциональна
expr -> name name (arg*,);
arg-> name ?[NUM]?; // тут мы будем проверять, есть ли далее скобка, так как она опциональна
*/

static std::vector<token_t> store;

token_t get_next_token();
kind_of_error parse_gatelevel_verilog();
kind_of_error parse_module(token_t&);
kind_of_error parse_decl(token_t&);
kind_of_error parse_expr(token_t&);
kind_of_error parse_assign_parts(token_t&);
kind_of_error parse_assign(token_t&);
kind_of_error parse_arg(token_t&);
kind_of_error parse_name_list(token_t&, token_t);



#define DEBUGTOKEN(tok, msg)\
printf("%s:%d: %s: token '%s' (%d)\n", __FILE__, __LINE__, (msg), yytext, (tok)) \

#define ASSERT_NEXT_TOKEN(var, tok, err) \
  do                                     \
  {                                      \
    var = get_next_token();              \
    if ((var) != (tok))                  \
    {                                     \
      std::cout<<__FILE__ <<__LINE__ << yytext <<" tok = " << tok << std::endl;                       \
      std::cout<<"Error (in Assert) = " << (err) << std::endl;   \
      return (err);                      \
    }                                    \
  } while (0)

static std::size_t place = 0;
static std::size_t line = 0;




class SymbolTable {
  public:
    SymbolTable() = default;
    ~SymbolTable() = default;
  
    void addSymbol(const std::string& name, SymbolType type);
    void removeSymbol(const std::string& name);
    SymbolType getSymbolType(const std::string& name);
  
    void setParentTable(SymbolTable* parentTable) {
      parentTable_ = parentTable;
    }
  
    SymbolTable* getParentTable() const {
      return parentTable_;
    }
  
  private:
    std::unordered_map<std::string, SymbolType> symbolTable_;
    SymbolTable* parentTable_ = nullptr;
};

// Описание методов
void SymbolTable::addSymbol(const std::string& name, SymbolType type) {
  symbolTable_[name] = type;
}

void SymbolTable::removeSymbol(const std::string& name) {
  symbolTable_.erase(name);
}

SymbolType SymbolTable::getSymbolType(const std::string& name) {
  if (symbolTable_.find(name) != symbolTable_.end()) {
    return symbolTable_[name];
  } else if (parentTable_ != nullptr) {
    return parentTable_->getSymbolType(name);
  } else {
    return SymbolType::VOID;
  }
}

// Добавление родительской таблицы
void addParentTable(SymbolTable* currentTable, SymbolTable* parentTable) 

{
  currentTable->setParentTable(parentTable);
}




// struct Namespace {
//     Namespace *parent;
//     std::unordered_map<std::string, SymbolType> symbols;
// };

// Namespace global;
// std::vector<Namespace *> table{&global};

// // Создание нового пространства имён
// if (MODULE) {
//     Namespace *nested = new Namespace();
//     nested->parent = table.back();
//     table.push_back(nested);
// }

// // Завершение определения модуля
// if (ENDMODULE) {
//     table.pop_back();
// }

// // Поиск символа в таблице символов
// Symbol findSymbol(const std::string &name, const std::vector<Namespace *> &table) {
//     int i = static_cast<int>(table.size()) - 1;
//     while (i >= 0) {
//         const auto &symbols = table[i]->symbols;
//         auto it = symbols.find(name);
//         if (it != symbols.end()) {
//             return it->second;
//         }
//         i--;
//     }
//     return SymbolNotFound;
// }






token_t get_next_token()
{ 
  token_t val = static_cast<token_t>( scan_token());
  std::cout << " type: "<< val << " tok: "<< yytext << " pl: " << place << " ln: " <<  yylineno << std::endl;
  place += 1;
  return val;
}

kind_of_error parse_gatelevel_verilog()
{
  
  token_t tok = START;
  kind_of_error rc = SUCCESS;
  while (tok != EOF_TOKEN)
  {
   DEBUGTOKEN(tok, "Verilog loop");
    ASSERT_NEXT_TOKEN(tok, MODULE, FAILURE_IN_GATE_LEVEL_VERILOG);
    rc = parse_module(tok);
  
  
  }
  std::cout << "Error! "<< "type = "<< rc << std::endl;
  return rc;
}

kind_of_error parse_module(token_t &tok)
{
  kind_of_error rc = SUCCESS;

  ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_MODULE_NAME);
  ASSERT_NEXT_TOKEN(tok, LBRACE, FAILURE_IN_MODULE_NAME);
  ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_MODULE_NAME);
  rc = parse_name_list(tok, RBRACE);
  line += 1;
  ASSERT_NEXT_TOKEN(tok, SEMICOLON, FAILURE_IN_MODULE_NAME);
  tok = get_next_token();
          
  while (rc == SUCCESS && tok != ENDMODULE)
  {
    DEBUGTOKEN(tok, "Module loop");
          
    switch (tok)
    {
    case INPUT:
    case OUTPUT:
    case WIRE:
      rc = parse_decl(tok);
      break;
    case ASSIGN:
      rc = parse_assign(tok);
      break;
    case STRING:
      rc = parse_expr(tok);
      DEBUGTOKEN(tok, "After expr");
      std::cout<<rc;
      break;   
    default:
      rc = FAILURE_IN_MODULE_INCAPTULATION;
      break;
    }
  }

  return rc;
}

kind_of_error parse_decl(token_t &tok)
{
  std::cout<< "asda"<< std::endl;
  tok = get_next_token();
  kind_of_error rc = SUCCESS;
  switch (tok)
  {
  case LBRACKET:
    ASSERT_NEXT_TOKEN(tok, NUM, FAILURE_IN_DECL);
    ASSERT_NEXT_TOKEN(tok, COLON, FAILURE_IN_DECL);
    ASSERT_NEXT_TOKEN(tok, NUM, FAILURE_IN_DECL);
    ASSERT_NEXT_TOKEN(tok, RBRACKET, FAILURE_IN_DECL);
    ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_DECL);
    rc = parse_name_list(tok, SEMICOLON);
    tok = get_next_token();
          
    line += 1;
    break;
  case STRING:
    rc = parse_name_list(tok, SEMICOLON);
    line += 1;
    tok = get_next_token();
    break;
  default:
    rc = FAILURE_IN_DECL;
    
    break;
  }
 
  return rc;
}

kind_of_error parse_expr(token_t &tok)
{
  kind_of_error rc = SUCCESS;

  ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_EXPR);
  ASSERT_NEXT_TOKEN(tok, LBRACE, FAILURE_IN_EXPR);
  line += 1;
  DEBUGTOKEN(tok, "Parse expr begin");
  rc = parse_arg(tok);
  DEBUGTOKEN(tok, "Parse expr end");
  return rc;
}

kind_of_error parse_assign_parts(token_t &tok)
{
kind_of_error rc = SUCCESS;
ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_ASSIGN);
tok = get_next_token();
switch (tok)
    {
    case LBRACKET:
    std::cout<<"aa"<< std::endl;
      ASSERT_NEXT_TOKEN(tok, NUM, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, COLON, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, NUM, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, RBRACKET, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, EQUALS, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, LFIGURNAYA, FAILURE_IN_ASSIGN);
      ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_ASSIGN);
      rc = parse_name_list(tok, RFIGURNAYA);
      std::cout<<__FILE__ <<__LINE__ <<yytext <<" end of lbracket case " <<"tok =" << tok << std::endl;
      break;
    case EQUALS:
      ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_ASSIGN);     
      std::cout<<__FILE__ <<__LINE__ <<yytext <<"end of equals case" <<"tok =" << tok << std::endl;
      break;
    default:
      break;
    }

  return rc;
}

kind_of_error parse_assign(token_t &tok)
{
  std::cout<< "fff"<< std::endl;
  kind_of_error rc = SUCCESS;
  
  
  while (tok != SEMICOLON)
  {
    rc = parse_assign_parts(tok);
    tok = get_next_token();  
    if (tok != COMMA && tok != SEMICOLON){
        rc = FAILURE_IN_ASSIGN;
    }   
    line += 1;
  }
  tok = get_next_token();
return rc;
}

kind_of_error parse_arg(token_t &tok)
{
  kind_of_error rc = SUCCESS;
  
  while (tok != RBRACE && rc == SUCCESS)
  {
    ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_ARG);
      store.push_back(tok);

    tok = get_next_token();
    DEBUGTOKEN(tok, "ARG  loop");

    if(tok == LBRACKET){
      ASSERT_NEXT_TOKEN(tok, NUM, FAILURE_IN_ARG);
      store.push_back(tok);
      ASSERT_NEXT_TOKEN(tok, RBRACKET, FAILURE_IN_ARG);
      tok = get_next_token();
    }
    if(tok != COMMA && tok != RBRACE){
      rc = FAILURE_IN_ARG; 
    }
  }
  ASSERT_NEXT_TOKEN(tok, SEMICOLON, FAILURE_IN_MODULE_INCAPTULATION);
  tok = get_next_token();
  return rc;
}

kind_of_error parse_name_list(token_t &tok, token_t separate_tok)
{
  kind_of_error rc = SUCCESS;
  
  while (tok  != separate_tok && rc == SUCCESS)
  {
    tok = get_next_token();
    switch (tok)
    {
    case COMMA:
      ASSERT_NEXT_TOKEN(tok, STRING, FAILURE_IN_PARSE_NAME_LIST);
      break;

    default:
      if(tok != separate_tok){
      
      rc = FAILURE_IN_PARSE_NAME_LIST; 
      }else{
        break;
      }
      break;
    }
  }
  return rc;
}

int main(int argc, char *argv[]) {
    yyin = fopen(argv[1], "r");
    return parse_gatelevel_verilog();
}