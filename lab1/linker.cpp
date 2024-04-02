// code written by Varun Deliwala
// vd2298 
// OS Lab 1 
// Prof Franke Hubertus

/*
                                                                               
                                     $$$$$$$@@@@@                              
                                 ############$$$$$$$$                          
                              #****!!!!*******####$$$$$#                       
                             *!!==!!!==!==!!!!!**######$##                     
                           !!!==;;;;::::;;===!!!!***#######                    
                          ====;::~~-----~~~:;;;=!!!!****###*                   
                          ==;;:~-,,.......,-~::;;!=!!!*******                  
                         ;=;;:~-,...........,-~:;;==!!*******!                 
                         ;;;::~,,....... ....,-~::;==!!!*****!                 
                         ;;;::~-,....      ...--~:;;==!!!!!!!!=                
                         ;;;;;::~-,..        .,-~:;;===!!!!!!=;                
                         :;=====;=!!=        .,-~::;;====!!===;                
                         ~;=!!!*#####*=     -~-~::;;;========;:                
                          :;!!*##$$@@@$#*!=;:::::;;;;======;;:                 
                           :=!**#$$@@@$$#*!==;;;;;;;;;;;;;;;:~                 
                            :=!!**##$$##**!!===;;;;;;;;;;;::~                  
                             ~;=!!!******!!!===;;;;;;;;:::~-                   
                              ,~;=!!!!!!!!====;;;;;::::~~-                     
                                .-~:;=====;;;;;::::~~~-,                       
                                   .,-~~::~~:~~~~--,,.                         
                                        .........                              
              
*/

#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <string>
#include <utility>

using namespace std;
/*
Global variables
*/
int line_number = 0;
int line_offset = 0;
int file_final_position = 0;
std::string line;
char *token_characters = NULL;
std::ifstream inputFile;
int largest_number = int(2E+30);
int special_Offset;
/*
Data Structures and related functions
*/

struct Symbol
{
public:
    std::string Symbol_Name;
    int Relative_Address;
    int Absolute_Address;
    int module_Number;
    bool used;
};

Symbol set_symbol(Symbol symbol, std::string Symbol_Name, int Relative_Address, int Module_Number)
{
    symbol.Symbol_Name = Symbol_Name;
    symbol.Relative_Address = Relative_Address;
    symbol.module_Number = Module_Number;
    symbol.used = false;
    return symbol;
}

std::map<std::string, int> map_Of_Symbols;
std::vector<int> module_Base_Table;

struct Token
{
    int line_number;
    int line_offset;
    std::string value;
};

Token set_values(Token Token, int line_number, int line_offset, std::string value)
{
    Token.line_number = line_number;
    Token.line_offset = line_offset;
    Token.value = value;
    return Token;
}

Token getToken()
{
    Token Temp;
    if (token_characters)
    {
        token_characters = std::strtok(NULL, " \t\n");
    }
    while (!token_characters)
    {
        if (inputFile.eof())
        {
            break;
        }
        else if (std::getline(inputFile, line))
        {
            int value = line.length();
            line_number = line_number + 1;
            line_offset = 1;
            token_characters = std::strtok(&line[0], " \n\t");
            if (inputFile.eof() && token_characters == NULL)
            {
                special_Offset = value;
            }
            else
            {
                special_Offset = value + 1;
            }
        }
    }
    if (!token_characters)
    {
        if (inputFile.eof())
        {
            line_offset = special_Offset;
            Temp = set_values(Temp, line_number, special_Offset, "");
        }
        else
        {

            Temp = set_values(Temp, line_number, line_offset, "");
        }
    }
    else
    {
        line_offset = token_characters - line.c_str() + 1;
        Temp = set_values(Temp, line_number, line_offset + 1, token_characters);
    }
    return Temp;
}

void __parseerror(int errorcode, int line_number, int line_offset)
{
    static std::string errstr[] = {
        "NUM_EXPECTED",           // Number expect, anything >= 2^30 is not a number either
        "SYM_EXPECTED",           // Symbol Expected
        "MARIE_EXPECTED",         // Addressing Expected which is A/E/I/R
        "SYM_TOO_LONG",           // Symbol Name is too long
        "TOO_MANY_DEF_IN_MODULE", // > 16
        "TOO_MANY_USE_IN_MODULE", // > 16
        "TOO_MANY_INSTR",         // total num_instr exceeds memory size (512)
    };
    std::string erorrCode = errstr[errorcode];
    cout << "Parse Error line " << line_number << " offset " << line_offset << ": " << erorrCode << endl;
    exit(1);
}

char readMARIE()
{
    Token temp;
    char Mode;
    temp = getToken();
    if (temp.value.length() != 1)
    {
        __parseerror(2, line_number, line_offset);
    }
    Mode = temp.value[0];
    if (Mode != 'M' && Mode != 'A' && Mode != 'R' && Mode != 'I' && Mode != 'E')
    {
        __parseerror(2, line_number, line_offset);
    }
    return Mode;
}

std::string readSymbol()
{
    Token temp;
    temp = getToken();
    if (temp.value.length() == 0)
    {
        __parseerror(1, line_number, line_offset); // Symbol expected
    }
    else if (temp.value.length() > 16)
    {
        __parseerror(3, line_number, line_offset); // Symbol length error
    }
    else if (!isalpha(temp.value[0]))
    {
        __parseerror(1, line_number, line_offset); // Symbol expected
    }
    else
    {
        for (int i = 0; i < temp.value.length(); i++)
        {
            if (!isalnum(temp.value[i]))
            {
                __parseerror(1, line_number, line_offset); // Symbol expected
            }
        }
    }
    return temp.value;
}

int readInt()
{
    int integer;
    Token temp;
    temp = getToken();
    if (temp.value == "")
    {

        __parseerror(0, line_number, line_offset);
    }
    else
    {
        for (int i = 0; i < temp.value.length(); i++)
        {
            if (isalpha(temp.value[i]))
                __parseerror(0, line_number, line_offset);
        }
        integer = std::stoi(temp.value);
        if (integer > largest_number)
        {

            __parseerror(0, line_number, line_offset);
        }
    }
    return integer;
}

void pass1()
{

    int starting_Address = 0;
    int instruction_Count = 0;
    int total_instructions = 0;
    int module_Count = -1;
    int module_address = 0;
    std::vector<pair<string, bool>> unique_Symbols; // true if repeated otherwise false
    while (inputFile.peek() != EOF && !inputFile.eof())
    {
        std::vector<pair<string, bool>> symbol_Encounters; // true if repeated otherwise false
        bool Repeated;
        Token token;
        int definition_Count = readInt();
        if (definition_Count < 0)
            exit(2);
        if (definition_Count > 16)
            __parseerror(4, line_number, line_offset); // Symbol expected

        module_address = module_address + instruction_Count;
        module_Count++;
        module_Base_Table.push_back(module_address);

        for (int i = 0; i < definition_Count; i++)
        {
            Symbol symbol;
            std::string symbol_Name = readSymbol();
            int relative_Address = readInt();
            symbol = set_symbol(symbol, symbol_Name, relative_Address, module_Count);
            // symbol.Absolute_Address = module_address + relative_Address; //initally zero + rel_addr
            if (map_Of_Symbols.find(symbol_Name) == map_Of_Symbols.end())
            {
                map_Of_Symbols[symbol_Name] = relative_Address; // initally the relative, will be changed later
                Repeated = false;
                unique_Symbols.push_back(make_pair(symbol_Name, false));
            }
            else
            {
                Repeated = true;
                for (int i = 0; i < unique_Symbols.size(); i++)
                {
                    if (unique_Symbols[i].first == symbol_Name)
                    {
                        unique_Symbols[i].second = true;
                    }
                }
            }

            symbol_Encounters.push_back(make_pair(symbol_Name, Repeated));
        }

        int use_Count = readInt();
        if (use_Count > 16)
            __parseerror(5, line_number, line_offset); // Symbol expected

        for (int i = 0; i < use_Count; i++)
        {
            std::string symbol_Name = readSymbol();
        }

        instruction_Count = readInt();
        total_instructions = total_instructions + instruction_Count;
        if (total_instructions > 512)
        {
            __parseerror(6, line_number, line_offset);
        }
        for (int i = 0; i < instruction_Count; i++)
        {
            char address_Mode = readMARIE();
            int operand = readInt();
        }
        for (int i = 0; i < definition_Count; i++)
        {
            if (!symbol_Encounters[i].second)
            {
                if (map_Of_Symbols[symbol_Encounters[i].first] >= instruction_Count)
                {
                    cout << "Warning: Module " << module_Count << ": " << symbol_Encounters[i].first << "=" << map_Of_Symbols[symbol_Encounters[i].first] << " valid=[0.." << instruction_Count - 1 << "] assume zero relative" << endl;
                    map_Of_Symbols[symbol_Encounters[i].first] = module_address + 0;
                }
                else
                {
                    map_Of_Symbols[symbol_Encounters[i].first] = module_address + map_Of_Symbols[symbol_Encounters[i].first];
                }
            }
            else
            {
                cout << "Warning: Module " << module_Count << ": " << symbol_Encounters[i].first << " redefinition ignored" << endl;
            }
        }
    }
    cout << "Symbol Table" << endl;
    for (int i = 0; i < unique_Symbols.size(); i++)
    {
        std::string error = "";
        if (unique_Symbols[i].second)
        {
            error = "Error: This variable is multiple times defined; first value used";
        }
        cout << unique_Symbols[i].first << "=" << map_Of_Symbols[unique_Symbols[i].first] << " " << error << endl;
    }
    cout << endl;
}

void pass2()
{   
    cout << "Memory Map" << endl;
    int starting_Address = 0;
    int module_Count = -1;
    int instruction_Count;
    int module_address = 0;
    int instruction_number = -1;
    std::map<std::string, int> symbol_and_module;
    map<string, int> symbol_use_map; 
    map<string, int> msmap;   
    vector<string> unique_symbols;       
    while (inputFile.peek() != EOF && !inputFile.eof())
    {
        Token token = getToken();
        // true if repeated otherwise false
        bool external_Ref;
        if (token.value.length() == 0)
            break;
        int definition_Count = std::stoi(token.value);
        if (definition_Count < 0)
            exit(2);
        module_Count++;
        module_address = module_Base_Table[module_Count];
        // Def count
        for (int i = 0; i < definition_Count; i++)
        {
            Symbol symbol;
            std::string symbol_Name = readSymbol();
            int relative_Address = readInt();
            if (symbol_use_map.find(symbol_Name) == symbol_use_map.end())
            {
                symbol_use_map[symbol_Name] = 0;
                msmap[symbol_Name] = module_Count;
                unique_symbols.push_back(symbol_Name);
            }
        }
        
        std::vector<pair<string, bool>> use_Encounters;
        int use_Count = readInt();
        for (int i = 0; i < use_Count; i++)
        {
            std::string symbol_Name = readSymbol();
            external_Ref = false;
            use_Encounters.push_back(make_pair(symbol_Name, external_Ref));
        }
        instruction_Count = readInt();
        for (int i = 0; i < instruction_Count; i++)
        {
            char address_Mode = readMARIE();
            instruction_number++;
            int instruction = readInt();
            int opCode = instruction / 1000;
            int operand = instruction % 1000;
            int Address = module_address + 1;
            std::string Errors = "";
            if (address_Mode == 'M')
            {
                if (opCode > 9)
                {
                    Errors = "Error: Illegal opcode; treated as 9999";
                    instruction = 9999;
                    continue;
                }
                else if (operand >= module_Base_Table.size())
                {
                    Errors = "Error: Illegal module operand ; treated as module=0";
                    operand = 0;
                }
                else
                {
                    operand = module_Base_Table[operand];
                }
                instruction = opCode * 1000 + operand;
            }
            else if (address_Mode == 'A')
            {
                if (opCode > 9)
                {
                    Errors = "Error: Illegal opcode; treated as 9999";
                    instruction = 9999;
                }
                else if (operand >= 512)
                {
                    Errors = "Error: Absolute address exceeds machine size; zero used";
                    operand = 0;
                    instruction = opCode * 1000;
                }
                else
                {
                    Errors = "";
                    instruction = instruction;
                }
            }
            else if (address_Mode == 'R')
            {
                if (opCode > 9)
                {
                    Errors = "Error: Illegal opcode; treated as 9999";
                    instruction = 9999;
                }
                else if (operand >= instruction_Count)
                {
                    Errors = "Error: Relative address exceeds module size; relative zero used";
                    instruction = (opCode * 1000) + module_address;
                }
                else
                {
                    if ((module_address + operand) >= 512)
                    {
                        Errors = "Error: Absolute address exceeds machine size; zero used";
                        instruction = opCode * 1000;
                    }
                    else
                    {
                        instruction = (opCode * 1000) + module_address + operand;
                    }
                }
            }
            else if (address_Mode == 'I')
            {
                if (opCode > 9)
                {
                    Errors = "Error: Illegal opcode; treated as 9999";
                    instruction = 9999;
                }
                else if (operand >= 900)
                {
                    Errors = "Error: Illegal immediate operand; treated as 999";
                    instruction = opCode * 1000 + 999;
                }
                else
                {
                    instruction = instruction;
                }
            }
            else if (address_Mode == 'E')
            {
                if (opCode > 9)
                {
                    Errors = "Error: Illegal opcode; treated as 9999";
                    instruction = 9999;
                }
                else if (operand >= use_Count)
                {
                    Errors = "Error: External operand exceeds length of uselist; treated as relative=0";
                    instruction = opCode * 1000 + module_address;
                }
                else
                {
                    use_Encounters[operand].second = true;
                    int temp_address;
                    if (map_Of_Symbols.find(use_Encounters[operand].first) == map_Of_Symbols.end())
                    {
                        Errors = "Error: " + use_Encounters[operand].first + " is not defined; zero used";
                        instruction = opCode * 1000;
                    }
                    else
                    {
                        Errors = "";
                        temp_address = map_Of_Symbols[use_Encounters[operand].first];
                        instruction = opCode * 1000 + temp_address;

                        string used_Symbol = use_Encounters[operand].first;
                        symbol_use_map[used_Symbol] = 1;
                    }
                }
            }
            printf("%03d: %04d %s \n", instruction_number, instruction, Errors.c_str());
        }
        for (int i = 0; i < use_Encounters.size(); i++)
        {
            if (!use_Encounters[i].second)
            {
                printf("Warning: Module %d: uselist[%d]=%s was not used\n", module_Count, i, use_Encounters[i].first.c_str());
            }
        }

    } 
    cout << endl;
    for (int i = 0; i < unique_symbols.size(); i++)
    {
        if (!symbol_use_map[unique_symbols[i]])
        {
            printf("Warning: Module %d: %s was defined but never used\n", msmap[unique_symbols[i]], unique_symbols[i].c_str());
        }
    }
}

int main(int argc, char *argv[])
{
    try
    {
        inputFile.open(argv[1]);
    }
    catch (std::exception &e)
    {
        std::cout << "Not a valid inputfile <" << argv[1] << "" << std::endl;
        return 1;
    }
    pass1();
    inputFile.close();
    inputFile.open(argv[1]);
    pass2();
    inputFile.close();
    cout << endl;
    return 0;
}
