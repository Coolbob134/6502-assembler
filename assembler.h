#include <string.h>
#include <stdio.h>


#define MAXINSTRUCTIONS 1024 


static char line[512];
static char directives[64][32] = {"ORG","FILL","ASCII","BYTE"};
static char instructions[MAXINSTRUCTIONS][32];
static char operands[MAXINSTRUCTIONS][64];
static char variableNames[512][32];
static char variableVals[512][32];

static int LineNums[MAXINSTRUCTIONS];
static int currentLine = 0;
static int currentaddr = 0x00;

int decodeline();
int getAddressMode(char operand[]);
int getInstructionLength(char operand[]);   //Returns the length of each line(in bytes)
int assemble(int argc, char **argv);
int addVar(char line[], int instructionCounter);
int insertVar(char line[], int instructionCounter);
int writeopcode(char instruction[], char operand[],unsigned char* memaddress);
int writeoperand(char operand[],unsigned char *memaddress);



int assemble(int argc, char **argv) //Call from main C file
{
    unsigned char* mem = (unsigned char*)malloc(0xFFFF*sizeof(char));

    FILE *infile, *outfile;

    for (int k = 0; k < 0xFFFF; k++)
    {
        *(mem+k) = 0;
    }

    if (argc >= 3)
    {
        infile = fopen(argv[1],"r");
        outfile = fopen(argv[2],"w");
    }
    else
    {
        infile = fopen("test.asm","r");
        outfile = fopen("programem.bin","w");
    }

    
    while (fgets(line,0x200,infile))//populates lines
    {
        currentLine++;
        if (line[0] != '\n' && line[0] != ';')
        {
            printf("%s",line);
            decodeline();
        }
    }
    fclose(infile);

    currentLine = 0;
    currentaddr = 0;

    while (instructions[currentLine][0] != '\0' && currentaddr <= 0xFFFF)
    {
        writeopcode(instructions[currentLine],operands[currentLine],mem + currentaddr);
        currentLine++;
    }

    fwrite(mem,sizeof(char),0xFFFF,outfile);
    fclose(outfile);
    
    

    return 0;
}

int decodeline()
{
    int instructioncounter = 0,opcodepos = 0,instructionpos = 0;
    char tempstr[16];

    for (int k = 0; k<=MAXINSTRUCTIONS; k++)
    {
        if (instructions[k][0] == '\0')
        {
            instructioncounter = k;
            break;
        }
    }

    for (int k = 0; line[k] != '\n';k++)    //removes comments
    {
        if (line[k] == ';')
        {
            line[k] = '\0';
            break;
        }
    }


    for (int k = 0; line[k] != '\n';k++)    //locates opcode
    {
        if (line[k] == '$' || line[k] == '\"' || line[k] == '(')
        {
            opcodepos = k;
            break;
        }
    }
    for (int k = 0; line[k] != '\n';k++)    //locates instruction
    {
        if (line[k] != '\t' && line[k] != ' ')
        {
            instructionpos = k;
            break;
        }
    }


    if (line[instructionpos] == '.')
    {
        
        switch (line[instructionpos+1])
        {
        case 'O':   //ORG
            strcpy(instructions[instructioncounter],".ORG");
            strncpy(operands[instructioncounter],line+opcodepos+1,5);
            LineNums[instructioncounter] = currentaddr;
            currentaddr = strtol(operands[instructioncounter],NULL,16);
            break;
        case 'B':   //BYTE
            strcpy(instructions[instructioncounter],".BYTE");
            opcodepos = 0;
            for (int k =0; line[k] != '\n';k++)
            {
                if (line[k] == '$')
                {
                    strncpy(tempstr,line+k+1,2);
                    operands[instructioncounter][opcodepos] = (unsigned char)strtol(tempstr,NULL,16);
                    opcodepos++;
                    k+= 2;
                }
            }
            LineNums[instructioncounter] = currentaddr;
            currentaddr+=opcodepos;
            break;
        case 'A':   //ASCII
            strcpy(instructions[instructioncounter],".ASCII");
            strcpy(operands[instructioncounter],line+opcodepos+1);
            operands[instructioncounter][strlen(operands[instructioncounter])-2] = '\0';
            LineNums[instructioncounter] = currentaddr;
            currentaddr += strlen(operands[instructioncounter]);
            break;
        
        default:
            break;
        }
    }
    else if (line[instructionpos] == ',')
    {
        addVar(line,instructioncounter);
    }
    else
    {
        strncpy(instructions[instructioncounter],line+instructionpos,3);
        if (opcodepos == 0)
        {
            insertVar(line,instructioncounter);
        }
        else
        {
            strcpy(operands[instructioncounter],line+opcodepos);
        }
        LineNums[instructioncounter] = currentaddr;
        currentaddr += getInstructionLength(operands[instructioncounter]);
    }
    return 0;
}

int getInstructionLength(char operand[])
{
    int addrmode = 0;
    addrmode = getAddressMode(operand);
    if (addrmode == 1 || addrmode == 2 || addrmode == 3 || addrmode == 12)
    {
        return 3;
    }
    if (addrmode == 4 || addrmode == 5 || addrmode == 6 || addrmode == 7 || addrmode == 8 || addrmode == 9)
    {
        return 2;
    }
    if (addrmode == 11)
    {
        return 1;
    }
    return 0;
}


int getAddressMode(char operand[])
{

    /*
        1   abs     Absolute                OPC $HHLL
        2   abs,X   Absolute,X-indexed      OPC $HHLL,X
        3   abs,Y   Absolute,Y-indexed      OPC $HHLL,Y
        4   #       Immediate               OPC #$BB
        5   zpg     Zeropage                OPC $LL
        6   zpg,X   Zeropage, X-indexed     OPC $LL,X
        7   zpg,Y   Zeropage, Y-indexed     OPC $LL,Y
        8   X,ind   X-indexed, indirect     OPC ($LL,X)
        9   ind,Y   indirect, Y-indexed     OPC ($LL),Y
        5   rel     relative                OPC $BB     //uses the same syntax as zpg, but different instruction
        11  impl    address implied         OPC         //same syntax as A, different instruction
        12  ind     address indirect        OPC ($HHLL)
        11  A       accumulator             OPC
    */

    int addrmode = 0;
    
    if (operand[0] == '#')
    {
        return 4;
    }
    if (operand[0] == '\0'||operand[0]==0xFF||operand[0]=='\n')
    {
        
        return 11;//13
    }
    if (operand[3]=='\0'||operand[3]=='\n')
    {
        return 5;//10
    }
    if (operand[3] == ',')
    {
        if (operand[4]=='X'||operand[4]=='x')
        {
            return 6;
        }
        if (operand[4]=='Y'||operand[4]=='y')
        {
            return 7;
        }
    }
    if (operand[5]=='\0'||operand[5]=='\n')
    {
        
        return 1;
    }
    if (operand[5]==',')
    {
        
        if (operand[6]=='X'||operand[6]=='x')
        {
            return 2;
        }
        if (operand[6]=='Y'||operand[6]=='y')
        {
            return 3;
        }
    }
    if (operand[0]=='(')
    {
        if (operand[5]=='X'||operand[5]=='x')
        {
            return 8;
        }
        if (operand[6]=='Y'||operand[6]=='y')
        {
            return 9;
        }
        if (operand[6]==')')
        {
            
            return 12;
        }
    }

    return -1;
}

int addVar(char line[], int instructionCounter)
{
    unsigned char vartype = 0,varfound = 0;
    int opcodepos = 0,varstart = 0,varend = 0,varnum = 0;
    char tempstr[16];

    for (int k = 0; k < 512;k++)
    {
        if (variableNames[k][0] == '\0')
        {
            varnum = k;
            break;
        }
    }

    for (int k = 0; k < strlen(line); k++)
    {
        if (varfound == 0 && line[k] != ' ' && line[k] != '\t')
        {
            varstart = k;
            varfound = 1;
        }
        if (line[k] == '=')
        {
            vartype = 1;
        }
        if ((line[k] == ' ' || line[k] == '=' || line[k] == ':') && varfound == 1 )
        {
            varend = k;
            varfound = 2;
        }
        if (line[k] == '$')
        {
            opcodepos = k;
            break;
        }
    }
    
    switch (vartype)
    {
    case 0: //label
        sprintf(variableVals[varnum],"$%04X",currentaddr);
        LineNums[instructionCounter] = currentaddr;
    break;
    case 1: //variable
        strncpy(variableVals[varnum],line+opcodepos,5);
        
        LineNums[instructionCounter] = currentaddr;
        break;
    
    default:
        break;
    }
    strncpy(variableNames[varnum],line+varstart+1,varend-varstart-1);
    strncpy(instructions[instructionCounter],line+varstart,varend-varstart);
    strcpy(operands[instructionCounter],variableVals[varnum]);

    return 0;
}

int insertVar(char line[], int instructionCounter)
{
    char variable[32],foundinstruction = 0;
    int varend = 0, varnum = 1024,varstart = 0;

    for (int k = 0; k <strlen(line);k++)
    {
        if (line[k] != ' ' && line[k] != '\t' && foundinstruction == 0)
        {
            foundinstruction = 1;
        }
        if ((line[k] == ' ' || line[k] == '\t')&&(foundinstruction == 1))
        {
            foundinstruction = 2;
        }
        if (line[k] != ' ' && line[k] != '\t' && foundinstruction == 2)
        {
            varstart = k;
            break;
        }
    }

    for (int k = varstart; k < strlen(line); k++)
    {
        if (line[k] == ' ' || line[k] == '\t' || line[k] == '\0' || line[k] == '\n')
        {
            varend = k;
            break;
        }
    }

    strncpy(variable,line+varstart,varend-varstart);
    variable[strlen(variable)-1] = '\0';
    varnum = 0;
    //printf("%s",variableNames[varnum]);
    while (variableNames[varnum][0] != '\0')
    {
        if (strcmp(variable,variableNames[varnum]) == 0)
        {
            strcpy(operands[instructionCounter],variableVals[varnum]);
            break;
        }
        varnum++;
    }
    

    return 0;
}

int writeoperand(char operand[],unsigned char* memaddress)
{
    int addressmode = 0;
    char tempstr[8] = {0};
    addressmode = getAddressMode(operand);
    switch (addressmode)
    {
    case 1://abs
        strncpy(tempstr,operand+1,2);
        *(memaddress+1) = (unsigned char)strtol(tempstr,NULL,16);
        strncpy(tempstr,operand+3,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr += 2;
        break;
        
    case 2://abs,X
        strncpy(tempstr,operand+1,2);
        *(memaddress+1) = (unsigned char)strtol(tempstr,NULL,16);
        strncpy(tempstr,operand+3,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr += 2;
        break;

    case 3://abs,Y
        strncpy(tempstr,operand+1,2);
        *(memaddress+1) = (unsigned char)strtol(tempstr,NULL,16);
        strncpy(tempstr,operand+3,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr += 2;
        break;
    
    case 4://I
        strncpy(tempstr,operand+2,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;
    
    case 5://ZPG
        strncpy(tempstr,operand+1,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;

    case 6://ZPG,X
        strncpy(tempstr,operand+1,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;

    case 7://ZPG,Y
        strncpy(tempstr,operand+1,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;

    case 8://X,ind
        strncpy(tempstr,operand+2,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;
    
    case 9://ind,Y
        strncpy(tempstr,operand+2,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr++;
        break;
    
    case 12://rel
        strncpy(tempstr,operand+2,2);
        *(memaddress+1) = (unsigned char)strtol(tempstr,NULL,16);
        strncpy(tempstr,operand+4,2);
        *(memaddress) = (unsigned char)strtol(tempstr,NULL,16);
        currentaddr += 2;
        break;


    default:
        break;
    }

    return 0;
}

int writeopcode(char instruction[], char operand[],unsigned char* memaddress)
{
    int instructionNo = 1024;

    char instructionArr[56][8] =  { "LDA","LDX","LDY","STA","STX","STY","ADC","SBC","INC","INX","INY","DEC","DEX","DEY","ASL","LSR","ROL","ROR","AND","ORA",
                                    "EOR","CMP","CPX","CPY","BIT","BCC","BCS","BNE","BEQ","BPL","BMI","BVC","BVS","TAX","TXA","TAY","TYA","TSX","TXS","PHA",
                                    "PLA","PHP","PLP","JMP","JSR","RTS","RTI","CLC","SEC","CLD","SED","CLI","SEI","CLV","BRK","NOP"};
    
    if (instruction[0] == '.')
    {
        switch (instruction[1])
        {
        case 'O'://.ORG
            currentaddr = (unsigned short int)strtol(operand,NULL,16);
            break;
        case 'B'://.BYTE
            for (int k = 0; k < strlen(operand);k++)
            {
                *(memaddress+k) = operand[k];
            }
            currentaddr += strlen(operand);
            break;
        case 'A'://.ASCII
            for (int k = 0; k < strlen(operand);k++)
            {
                *(memaddress+k) = operand[k];
            }
            currentaddr += strlen(operand);
            break;
        default:
            break;
        }
        return 0;
    }
    if (instruction[0] == ',')
    {
        return 0;
    }

    for (int k = 0; k<56;k++)
    {
        if (strcmp(instruction,instructionArr[k]) == 0)
        {
            instructionNo = k;
            break;
        }
    }

    writeoperand(operand,memaddress+1);
    
    currentaddr++;
    switch (instructionNo)
    {
        case 0: //LDA

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xAD;
            break;
        case 2: //abs,X
            *memaddress = 0xBD;
            break;
        case 3: //abs,Y
            *memaddress = 0xB9;
            break;
        case 4: //#
            *memaddress = 0xA9;
            break;
        case 5: //zpg
            *memaddress = 0xA5;
            break;
        case 6: //zpg,X
            *memaddress = 0xB5;
            break;
        case 8: //X,ind
            *memaddress = 0xA1;
            break;
        case 9: //ind,Y
            *memaddress = 0xB1;
            break;
        default:
            return -1;
            break;
        }
        break;
    
    case 1: //LDX
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xAE;
            break;
        case 3: //abs,Y
            *memaddress = 0xBE;
            break;
        case 4: //#
            *memaddress = 0xA2;
            break;
        case 5: //zpg
            *memaddress = 0xA6;
            break;
        case 7: //zpg,Y
            *memaddress = 0xB6;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 2: //LDY
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xAC;
            break;
        case 2: //abs,X
            *memaddress = 0xBC;
            break;
        case 4: //#
            *memaddress = 0xA0;
            break;
        case 5: //zpg
            *memaddress = 0xA4;
            break;
        case 6: //zpg,X
            *memaddress = 0xB4;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 3: //STA
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x8D;
            break;
        case 2: //abs,X
            *memaddress = 0x9D;
            break;
        case 3: //abs,Y
            *memaddress = 0x99;
            break;
        case 5: //zpg
            *memaddress = 0x85;
            break;
        case 6: //zpg,X
            *memaddress = 0x95;
            break;
        case 8: //X,ind
            *memaddress = 0x81;
            break;
        case 9: //ind,Y
            *memaddress = 0x91;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 4: //STX
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x8E;
            break;
        case 5: //zpg
            *memaddress = 0x86;
            break;
        case 7: //zpg,Y
            *memaddress = 0x96;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 5: //STY
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x8C;
            break;
        case 5: //zpg
            *memaddress = 0x84;
            break;
        case 6: //zpg,X
            *memaddress = 0x94;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 6: //ADC
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x6D;
            break;
        case 2: //abs,X
            *memaddress = 0x7D;
            break;
        case 3: //abs,Y
            *memaddress = 0x79;
            break;
        case 4: //#
            *memaddress = 0x69;
            break;
        case 5: //zpg
            *memaddress = 0x65;
            break;
        case 6: //zpg,X
            *memaddress = 0x75;
            break;
        case 8: //X,ind
            *memaddress = 0x61;
            break;
        case 9: //ind,Y
            *memaddress = 0x71;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 7: //SBC
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xED;
            break;
        case 2: //abs,X
            *memaddress = 0xFD;
            break;
        case 3: //abs,Y
            *memaddress = 0xF9;
            break;
        case 4: //#
            *memaddress = 0xE9;
            break;
        case 5: //zpg
            *memaddress = 0xE5;
            break;
        case 6: //zpg,X
            *memaddress = 0xF5;
            break;
        case 8: //X,ind
            *memaddress = 0xE1;
            break;
        case 9: //ind,Y
            *memaddress = 0xF1;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 8: //INC
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xEE;
            break;
        case 2: //abs,X
            *memaddress = 0xFE;
            break;
        case 5: //zpg
            *memaddress = 0xE6;
            break;
        case 6: //zpg,X
            *memaddress = 0xF6;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 9: //INX
        switch (getAddressMode(operand))
        {
        case 11: //impl
            *memaddress = 0xE8;
            break;
        default:
            return -1;
            break;
        }
        break;

    
    case 10: //INY
        switch (getAddressMode(operand))
        {
        case 11: //impl
            *memaddress = 0xC8;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 11: //DEC
        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xCE;
            break;
        case 2: //abs,X
            *memaddress = 0xDE;
            break;
        case 5: //zpg
            *memaddress = 0xC6;
            break;
        case 6: //zpg,X
            *memaddress = 0xD6;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 12: //DEX
        switch (getAddressMode(operand))
        {
        case 11: //impl
            *memaddress = 0xCA;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 13: //DEY
        switch (getAddressMode(operand))
        {
        case 11: //impl
            *memaddress = 0x88;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 14: //ASL

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x0E;
            break;
        case 2: //abs,X
            *memaddress = 0x1E;
            break;
        case 5: //zpg
            *memaddress = 0x06;
            break;
        case 6: //zpg,X
            *memaddress = 0x16;
            break;
        case 11: //A
            *memaddress = 0x0A;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 15: //LSR

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x4E;
            break;
        case 2: //abs,X
            *memaddress = 0x5E;
            break;
        case 5: //zpg
            *memaddress = 0x46;
            break;
        case 6: //zpg,X
            *memaddress = 0x56;
            break;
        case 11: //A
            *memaddress = 0x4A;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 16: //ROL

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x2E;
            break;
        case 2: //abs,X
            *memaddress = 0x3E;
            break;
        case 5: //zpg
            *memaddress = 0x26;
            break;
        case 6: //zpg,X
            *memaddress = 0x36;
            break;
        case 11: //A
            *memaddress = 0x2A;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 17: //ROR

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x6E;
            break;
        case 2: //abs,X
            *memaddress = 0x7E;
            break;
        case 5: //zpg
            *memaddress = 0x66;
            break;
        case 6: //zpg,X
            *memaddress = 0x76;
            break;
        case 11: //A
            *memaddress = 0x6A;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 18: //AND

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x2D;
            break;
        case 2: //abs,X
            *memaddress = 0x3D;
            break;
        case 3: //abs,Y
            *memaddress = 0x39;
            break;
        case 4: //#
            *memaddress = 0x29;
            break;
        case 5: //zpg
            *memaddress = 0x25;
            break;
        case 6: //zpg,X
            *memaddress = 0x35;
            break;
        case 8: //X,ind
            *memaddress = 0x21;
            break;
        case 9: //ind,Y
            *memaddress = 0x31;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 19: //ORA

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x0D;
            break;
        case 2: //abs,X
            *memaddress = 0x1D;
            break;
        case 3: //abs,Y
            *memaddress = 0x19;
            break;
        case 4: //#
            *memaddress = 0x09;
            break;
        case 5: //zpg
            *memaddress = 0x05;
            break;
        case 6: //zpg,X
            *memaddress = 0x15;
            break;
        case 8: //X,ind
            *memaddress = 0x01;
            break;
        case 9: //ind,Y
            *memaddress = 0x11;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 20: //EOR

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x4D;
            break;
        case 2: //abs,X
            *memaddress = 0x5D;
            break;
        case 3: //abs,Y
            *memaddress = 0x59;
            break;
        case 4: //#
            *memaddress = 0x49;
            break;
        case 5: //zpg
            *memaddress = 0x45;
            break;
        case 6: //zpg,X
            *memaddress = 0x55;
            break;
        case 8: //X,ind
            *memaddress = 0x41;
            break;
        case 9: //ind,Y
            *memaddress = 0x51;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 21: //CMP

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xCD;
            break;
        case 2: //abs,X
            *memaddress = 0xDD;
            break;
        case 3: //abs,Y
            *memaddress = 0xD9;
            break;
        case 4: //#
            *memaddress = 0xC9;
            break;
        case 5: //zpg
            *memaddress = 0xC5;
            break;
        case 6: //zpg,X
            *memaddress = 0xD5;
            break;
        case 8: //X,ind
            *memaddress = 0xC1;
            break;
        case 9: //ind,Y
            *memaddress = 0xD1;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 22: //CPX

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xEC;
            break;
        case 4: //#
            *memaddress = 0xE0;
            break;
        case 5: //zpg
            *memaddress = 0xE4;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 23: //CPY

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0xCC;
            break;
        case 4: //#
            *memaddress = 0xC0;
            break;
        case 5: //zpg
            *memaddress = 0xC4;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 24: //BIT

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x2C;
            break;
        // case 4: //#
        //     *memaddress = 0x89;
        //     break;
        case 5: //zpg
            *memaddress = 0x24;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 25: //BCC

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0x90;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 26: //BCS

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0xB0;
            break;
        default:
            return -1;
            break;
        }
        break;
        

    case 27: //BNE

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0xD0;
            break;
        default:
            return -1;
            break;
        }
        break;
        

    case 28: //BEQ

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0xF0;
            break;
        default:
            return -1;
            break;
        }
        break;
        

    case 29: //BPL

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0x10;
            break;
        default:
            return -1;
            break;
        }
        break;
        

    case 30: //BMI

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0x30;
            break;
        default:
            return -1;
            break;
        }
        break;
            

    case 31: //BVC

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0x50;
            break;
        default:
            return -1;
            break;
        }
        break;
            

    case 32: //BVS

        switch (getAddressMode(operand))
        {
        case 5: //rel
           *memaddress = 0x70;
            break;
        default:
            return -1;
            break;
        }
        break;
            

    case 33: //TAX

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xAA;
            break;
        default:
            return -1;
            break;
        }
        break;
                

    case 34: //TXA

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x8A;
            break;
        default:
            return -1;
            break;
        }
        break;
                

    case 35: //TAY

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xA8;
            break;
        default:
            return -1;
            break;
        }
        break;
                    

    case 36: //TYA

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x98;
            break;
        default:
            return -1;
            break;
        }
        break;
               

    case 37: //TSX

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xBA;
            break;
        default:
            return -1;
            break;
        }
        break;

                    

    case 38: //TXS

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x9A;
            break;
        default:
            return -1;
            break;
        }
        break;
                    

    case 39: //PHA

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x48;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 40: //PLA

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x68;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 41: //PHP

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x08;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 42: //PLP

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x28;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 43: //JMP

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x4C;
            break;
        case 12: //ind
            *memaddress = 0x6C;
            break;
        default:
            return -1;
            break;
        }
        break;
        

    case 44: //JSR

        switch (getAddressMode(operand))
        {
        case 1: //abs
            *memaddress = 0x20;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 45: //RTS

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x60;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 46: //RTI

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x40;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 47: //CLC

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x18;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 48: //SEC

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x38;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 49: //CLD

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xD8;
            break;
        default:
            return -1;
            break;
        }
        break;
                        

    case 50: //SED

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xF8;
            break;
        default:
            return -1;
            break;
        }
        break;


    case 51: //CLI

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x58;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 52: //SEI

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x78;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 53: //CLV

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xB8;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 54: //BRK

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0x00;
            break;
        default:
            return -1;
            break;
        }
        break;
    

    case 55: //NOP

        switch (getAddressMode(operand))
        {
        case 11: //impl
           *memaddress = 0xEA;
            break;
        default:
            return -1;
            break;
        }
        break;

    default:
        break;
    }



}

