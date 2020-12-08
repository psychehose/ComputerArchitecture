#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

/*
 * For debug option. If you want to debug, set 1.
 * If not, set 0.
 */
#define DEBUG 0

#define MAX_SYMBOL_TABLE_SIZE   1024
#define MEM_TEXT_START          0x00400000
#define MEM_DATA_START          0x10000000
#define BYTES_PER_WORD          4
#define INST_LIST_LEN           20

/******************************************************
 * Structure Declaration
 *******************************************************/

typedef struct inst_struct {
    char *name;
    char *op;
    char type;
    char *funct;
} inst_t;

typedef struct symbol_struct {
    char name[32];
    uint32_t address;
} symbol_t;

enum section { 
    DATA = 0,
    TEXT,
    MAX_SIZE
};
int text_count = 0;
int data_count = 0;

/******************************************************
 * Global Variable Declaration
 *******************************************************/

inst_t inst_list[INST_LIST_LEN] = {       //  idx
    {"addiu",   "001001", 'I', ""},       //    0
    {"addu",    "000000", 'R', "100001"}, //    1
    {"and",     "000000", 'R', "100100"}, //    2
    {"andi",    "001100", 'I', ""},       //    3
    {"beq",     "000100", 'I', ""},       //    4
    {"bne",     "000101", 'I', ""},       //    5
    {"j",       "000010", 'J', ""},       //    6
    {"jal",     "000011", 'J', ""},       //    7
    {"jr",      "000000", 'R', "001000"}, //    8
    {"lui",     "001111", 'I', ""},       //    9
    {"lw",      "100011", 'I', ""},       //   10
    {"nor",     "000000", 'R', "100111"}, //   11
    {"or",      "000000", 'R', "100101"}, //   12
    {"ori",     "001101", 'I', ""},       //   13
    {"sltiu",   "001011", 'I', ""},       //   14
    {"sltu",    "000000", 'R', "101011"}, //   15
    {"sll",     "000000", 'R', "000000"}, //   16
    {"srl",     "000000", 'R', "000010"}, //   17
    {"sw",      "101011", 'I', ""},       //   18
    {"subu",    "000000", 'R', "100011"}  //   19
};

symbol_t SYMBOL_TABLE[MAX_SYMBOL_TABLE_SIZE]; // Global Symbol Table

uint32_t symbol_table_cur_index = 0; // For indexing of symbol table

/* Temporary file stream pointers */
FILE *data_seg;
FILE *text_seg;

/* Size of each section */
uint32_t data_section_size = 0;
uint32_t text_section_size = 0;

/******************************************************
 * Function Declaration
 *******************************************************/
void Eliminate(char *str, char ch)
{
    for (; *str != '\0'; str++)//종료 문자를 만날 때까지 반복
    {
        if (*str == ch)//ch와 같은 문자일 때
        {
            strcpy(str, str + 1);
            str--;
        }
    }
}





/* Change file extension from ".s" to ".o" */
char* change_file_ext(char *str) {
    char *dot = strrchr(str, '.');

    if (!dot || dot == str || (strcmp(dot, ".s") != 0))
        return NULL;

    str[strlen(str) - 1] = 'o';
    return "";
}


/* Add symbol to global symbol table */
void symbol_table_add_entry(symbol_t symbol)
{
    SYMBOL_TABLE[symbol_table_cur_index++] = symbol;
#if DEBUG
    printf("%s: 0x%08x\n", symbol.name, symbol.address);
#endif
}

/* Convert integer number to binary string */
char* num_to_bits(unsigned int num, int len) 
{
    char* bits = (char *) malloc(len+1);
    int idx = len-1, i;
    while (num > 0 && idx >= 0) {
        if (num % 2 == 1) {
            bits[idx--] = '1';
        } else {
            bits[idx--] = '0';
        }
        num /= 2;
    }
    for (i = idx; i >= 0; i--){
        bits[i] = '0';
    }
    bits[len] = '\0';
    return bits;
}

void record_data_section(FILE *output)
{
    uint32_t cur_addr = MEM_DATA_START;
    char line[1024];
    long decimal;
    /* Point to data segment stream */
    rewind(data_seg);
//    fopen(output,"w");

    /* Print .data section */
    while (fgets(line, 1024, data_seg) != NULL) {
        char *temp;
        char _line[1024] = {0};

        strcpy(_line,line);

        temp = strtok(_line,"\t\n");
        while (temp != NULL) {
//
            if (strstr(temp, ".word") == NULL) {
                if(strstr(temp,"0x")==NULL) {
                    decimal = strtol(temp, NULL, 10);
                }
                else{
                    decimal = strtol(temp, NULL, 16);
                }
                strcpy(line,temp);
            }
            temp = strtok(NULL, "\t\n");
        }
//        printf("%d\n",decimal);


//        char hexadecimal[1024] ={0};
        char hexadecimal2[1024] ={0};
        strcpy(hexadecimal2, num_to_bits(decimal,32));
        //output file에 w모드로 쓰면 됨.

        fprintf(output,"%s\n",hexadecimal2);



#if DEBUG


//        printf("0x%08x: ", cur_addr);
//        printf("%s\n",line);
//        printf("0x%08x\n", decimal);
//        printf("%s\n",hexadecimal2);
//        printf("%d\n",strlen(hexadecimal));
#endif
        cur_addr += BYTES_PER_WORD;
    }
}
void record_text_section(FILE *output)
{
    uint32_t cur_addr = MEM_TEXT_START;
    char line[1024]={0};

    /* Point to text_seg stream */
    rewind(text_seg);

    /* Print .text section */
    while (fgets(line, sizeof(line), text_seg) != NULL) {

//        char inst[0x1000] = {0};
        char op[32] = {0};
//        char label[32] = {0};
//        char funct[32]={0};
        char type = '0';
//        int i, idx = 0;

        int rs, rt, rd, imm, shamt;
        int addr;
        int target = 0; // INST_LIST_LEN의 target

        rs = rt = rd = imm = shamt = addr = 0;

        char *temp_text;
        char _line[1024] = {0};

        strcpy(_line,line);



        temp_text = strtok(_line," ");

        for(int i =0; i<INST_LIST_LEN; i++) {
            if(strcmp(temp_text,inst_list[i].name)==0) {
                type = inst_list[i].type;
                target = i;
                break;
            }
        }



#if DEBUG
        //        printf("0x%08x: ", cur_addr);
#endif
        /* Find the instruction type that matches the line */
        /* blank */

        switch (type) {
            case 'R': {
                char result[1024]={0};
                int index = 0;
                char funct[32]={0};
                char rd_str[32]={0};
                char rs_str[32]={0};
                char rt_str[32]={0};
                char shamt_str[32]={0};
                strcpy(op, inst_list[target].op);
                strcpy(funct, inst_list[target].funct);
                // 분리/ (rs rt rd 구분 index 이용)
                while (temp_text != NULL) {

                    if (strstr(temp_text, "$") != NULL) { //$ 있는 경우.

                        if (index == 0) {
                            strcpy(rd_str,temp_text+1);
                        }
                        else if (index == 1) {
                            strcpy(rs_str,temp_text+1);
                        }
                        else if (index == 2) {
                            strcpy(rt_str,temp_text+1);
                        }

                        index = index + 1;
                    }

                    else if (index == 1) {
                        strcpy(rs_str,rd_str);
                        strcpy(shamt_str,rt_str);
                        strcpy(rd_str,"0");
                        strcpy(rt_str,"0");
                    }


                    else if (index == 2 ){
                            strcpy(rt_str,temp_text);
                            strcpy(shamt_str,rt_str);
                            strcpy(rt_str,rs_str);
                            strcpy(rs_str,"0");
                    }

                    temp_text = strtok(NULL, " ,\n");
                }

                if(index == 1) {

                    strcpy(rs_str,rd_str);
                    strcpy(shamt_str,rt_str);
                    strcpy(rd_str,"0");
                    strcpy(rt_str,"0");
                }


                int op_int = strtol(op,NULL,2);
                int funct_int= strtol(funct,NULL,2);
//                printf("sfsf %d\n",funct_int);

                rs = strtol(rs_str,NULL,10);
                rt = strtol(rt_str,NULL,10);
                rd = strtol(rd_str,NULL,10);
                shamt = strtol(shamt_str,NULL,10);

                strcpy(op, num_to_bits(op_int,6));
                strcpy(rs_str, num_to_bits(rs,5));
                strcpy(rt_str, num_to_bits(rt,5));
                strcpy(rd_str, num_to_bits(rd,5));
                strcpy(shamt_str,num_to_bits(shamt,5));
                strcpy(funct, num_to_bits(funct_int,6));

                strcat(result,op);
                strcat(result,rs_str);
                strcat(result,rt_str);
                strcat(result,rd_str);
                strcat(result,shamt_str);
                strcat(result,funct);

//                printf("r타입:\n");
//                printf("%s\n",result);

                fprintf(output,"%s\n",result);


#if DEBUG
                //                printf("op:%s rs:$%d rt:$%d rd:$%d shamt:%d funct:%s\n",
//                       op, rs, rt, rd, shamt, inst_list[idx].funct);
#endif
                break;
            }
            case 'I': {
                char result[1024]={0};
                int index = 0;
                char rs_str[32]={0};
                char rt_str[32]={0};
                char imm_or_addr[32]={0};
                strcpy(op, inst_list[target].op);
                // 분리/ (rs rt rd 구분 index 이용)


                while (temp_text != NULL) {

                    if (strstr(temp_text, "$") != NULL) { //$ 있는 경우.

                        if (index == 0) {
                            strcpy(rt_str,temp_text+1);
                        }
                        else if (index == 1) {
                            strcpy(rs_str,temp_text+1);
                        }
                        if(strstr(temp_text, "(")){

                            if (index == 1) { //여기 수정하기. %str +1 하면 될 거 같음.
                                char imm_or_addr_temp[1024]= {0};

                                strcpy(rs_str," ");
                                char *ptrFind;

                                ptrFind = strstr(temp_text,"$");
                                strncpy(imm_or_addr_temp,temp_text, ptrFind-temp_text-1);
                                strncpy(rs_str,ptrFind+1,1);

//                                printf("%s\n",imm_or_addr_temp);
                                strcpy(imm_or_addr,imm_or_addr_temp);
                            }
                        }

                        index = index + 1;
                    }
                    else{ //없는 경
                        strcpy(imm_or_addr,temp_text);
                    }
                    temp_text = strtok(NULL, " ,\n");
                }

                int op_int = strtol(op,NULL,2);
                int imm_or_addr_int = 0;

                if(strstr(imm_or_addr,"0x")==NULL){ // 16진수 아닌 경우
                    if(isdigit(imm_or_addr[0]) != 0 || imm_or_addr[0] =='-') {

                        imm_or_addr_int = strtol(imm_or_addr, NULL, 10);
                    }
                    else {
//                        printf("%s",imm_or_addr); //라벨인경우 테이블에서 찾기
                        int j = 0;

                        while(1) {
                            if(strcmp(imm_or_addr,SYMBOL_TABLE[j].name)==0) {
//                                printf("cur_addr:0x%08x\n",cur_addr);
                                imm_or_addr_int = SYMBOL_TABLE[j].address - ( cur_addr +4 );
//                                printf("뺀값: 0x%08x\n",imm_or_addr_int);
                                imm_or_addr_int = imm_or_addr_int/4;
//                                printf("0x%08x\n",imm_or_addr_int);
//                                printf("%d\n",imm_or_addr_int);
                                char temp_str[1024] ={0};
                                strcpy(temp_str,rs_str);
                                strcpy(rs_str,rt_str);
                                strcpy(rt_str,temp_str);
                                break;
                            }
                            j++;
                        }

                    }
//                    imm_or_addr_int = strtol(imm_or_addr, NULL, 10);
                }
                else if (strstr(imm_or_addr,"0x")){
                    imm_or_addr_int = strtol(imm_or_addr,NULL,16);
                }

                index = 0;
                rs = strtol(rs_str,NULL,10);
                rt = strtol(rt_str,NULL,10);

                strcpy(op, num_to_bits(op_int,6));
                strcpy(rs_str, num_to_bits(rs,5));
                strcpy(rt_str, num_to_bits(rt,5));
                strcpy(imm_or_addr, num_to_bits(imm_or_addr_int,16));
                strcat(result,op);
                strcat(result,rs_str);
                strcat(result,rt_str);
                strcat(result,imm_or_addr);


//                printf("i타입:\n");
                fprintf(output,"%s\n",result);



                /* blank */
#if DEBUG
                //                printf("op:%s rs:$%d rt:$%d imm:0x%x\n",
//                       op, rs, rt, imm);
#endif
                break;
            }

            case 'J': {

                char result[1024] = {0};
//                int index = 0;
                int imm_or_addr_int = 0;
                char onlyaddr[32] = {0};
                strcpy(op, inst_list[target].op);
                temp_text = strtok(NULL, "\t\n");
                strcpy(onlyaddr,temp_text);

                int op_int = strtol(op, NULL, 2);

//                printf("%s\n",temp_text);

                int j = 0;
                while (1) {
                    if (strcmp(onlyaddr, SYMBOL_TABLE[j].name) == 0) {
//                        printf("cur_addr:0x%08x\n", cur_addr);
                        imm_or_addr_int = SYMBOL_TABLE[j].address;
//                        printf(": 0x%08x\n", imm_or_addr_int);
                        imm_or_addr_int = imm_or_addr_int / 4;
                        imm_or_addr_int = 0x03ffffff & imm_or_addr_int;

//                        printf("tesT: 0x%08x\n",imm_or_addr_int);
                        break;
                    }
                    j++;
                }
                strcpy(op, num_to_bits(op_int,6));
                strcpy(onlyaddr, num_to_bits(imm_or_addr_int,26));
                strcat(result,op);
                strcat(result,onlyaddr);
//                printf("j타입:\n");
                fprintf(output,"%s\n",result);




#if DEBUG
                //                printf("op:%s addr:%i\n", op, addr);
#endif
                break;
            }
            default:
                break;
        }

        //fprintf(output, "\n");




        cur_addr += BYTES_PER_WORD;


        memset(line, 0, sizeof(line));
    }
}







/* Fill the blanks */
void make_binary_file(FILE *output)
{

    int data_size = data_count*4;
    int text_size = text_count*4;

    char str_data_size[1024]= {0};
    char str_text_size[1024]= {0};

    strcpy(str_data_size,num_to_bits(data_size,32));
    strcpy(str_text_size,num_to_bits(text_size,32));


    fprintf(output,"%s\n",str_text_size);
    fprintf(output,"%s\n",str_data_size);



#if DEBUG
    char line[1024] = {0};
    rewind(text_seg);
    /* Print line of text segment */
    while (fgets(line, 1024, text_seg) != NULL) {
        printf("%s",line);
    }
    printf("text section size: %d, data section size: %d\n",
            text_section_size, data_section_size);
#endif

    /* Print text section size and data section size */
    /* blank */

    /* Print .text section */
    record_text_section(output);

    /* Print .data section */
    record_data_section(output);
}

/* Fill the blanks */
void make_symbol_table(FILE *input)
{
    char line[1024] = {0};
    uint32_t address = 0;
    enum section cur_section = MAX_SIZE;

    /* Read each section and put the stream */
    while (fgets(line, 1024, input) != NULL) { //input file에서 한줄씩 받아옴.
        char *temp;
        char _line[1024] = {0};
        symbol_t temp_symbol;


        strcpy(_line, line);

        temp = strtok(_line, "\t\n");

        /* Check section type */
        if (!strcmp(temp, ".data")) { // return 1 -> .data
            /* blank */
            address = 0x10000000;
            data_seg = tmpfile();
            cur_section = DATA;
            continue;
        }
        else if (!strcmp(temp, ".text")) {
            /* blank */
            address= 0x400000;
            text_seg = tmpfile();
            cur_section = TEXT;
            continue;
        }

        /* Put the line into each segment stream */
        if (cur_section == DATA) {
            Eliminate(temp,':');
            data_count++;

            temp_symbol.address = address;
            strcpy(temp_symbol.name,temp);
            symbol_table_add_entry(temp_symbol);
            while (temp != NULL)
            {
                temp = strtok(NULL, "\n");
                if(temp != NULL){

                    fputs(temp,data_seg);
                    fputs("\n",data_seg);
                }
            }
        }

        else if (cur_section == TEXT) {

            if(strchr(temp,':') != NULL) {
                Eliminate(temp,':');
//                fputs("labelMark\n",text_seg);
                temp_symbol.address = address;
                strcpy(temp_symbol.name,temp);
                symbol_table_add_entry(temp_symbol);
                address -=BYTES_PER_WORD;
            }

                //(strchr(temp,':') != NULL)
            else {
//                printf("\n%s ",temp); // ISA 명령어 and, la 추출

                text_count ++;
                if(strstr(temp,"la") != NULL){//temp가 la 일 경우
                    char temp_la_list[64];
                    uint32_t data_address;
                    ///////la -> lui and ori
                    while (temp != NULL){
                        temp = strtok(NULL, "\n");


                        if(temp != NULL) {
                            strcpy(temp_la_list,temp);
                            char *findPtr = strstr(temp_la_list,",");


                            int i =0;
                            while(1){
                                if(!strcmp(SYMBOL_TABLE[i].name, findPtr+2)){
                                    data_address = SYMBOL_TABLE[i].address;
                                    break;
                                }
                                i++;
                            }
//                            printf("%s:",findPtr+2); // 데이터 변수 출력 예를들면 data1, data2.
//                            printf("0x%08x\n",data_address);
//                            printf("\t%s\n",temp);

                            char transLa[1024] ={0};
                            char data_address_string[1024] = {0};
                            char upper[1024] = {0};
                            char temp_lower[1024]= {0};;
                            char lower[1024] = {0};
                            char *upperlower;

                            sprintf(data_address_string, "0x%08x", data_address);
                            upperlower = data_address_string + 6;

//                            printf("test: %s\n",data_address_string);
                            strcpy(transLa,temp);
                            char *findPtr_reg = strstr(transLa,",");
                            char reg[1024];
                            strcpy(findPtr_reg,"");
                            strncpy(upper,data_address_string,6);
                            strncpy(temp_lower,upperlower,4);
                            strcpy(lower,"0x");
                            strcpy(lower+2,temp_lower);
                            strncpy(reg,transLa,2);

//                            printf("%s\n",transLa);
//                            strcat(transLa,"");
//                            printf("test2:%s\n",upper);
//                            printf("test3:%s\n",lower);

                            if(!strcmp(temp_lower,"0000")){
                                fputs("lui ",text_seg);
                                fputs(transLa,text_seg);
                                fputs(", ",text_seg);
                                fputs(upper,text_seg);
                                fputs("\n",text_seg);
                            }
                            else{
                                fputs("lui ",text_seg);
                                fputs(transLa,text_seg);
                                fputs(", ",text_seg);
                                fputs(upper,text_seg);
                                fputs("\n",text_seg);

                                fputs("ori ", text_seg);
                                fputs(transLa,text_seg);
                                fputs(", ",text_seg);
                                fputs(transLa,text_seg);
                                fputs(", ",text_seg);
                                fputs(lower,text_seg);
                                fputs("\n",text_seg);
                                address += BYTES_PER_WORD;
                                text_count ++;
                            }

                            break;
                        }
                    }
                }
                else if(strstr(temp,"lw") != NULL || strstr(temp,"sw") != NULL  ) { // lw나 sw인 경우

                    fputs(temp,text_seg);
                    fputs(" ", text_seg);
                    while (temp != NULL ){
                        temp = strtok(NULL,"\n");

                        if (temp != NULL) {

                            fputs(temp,text_seg);
                            fputs("\n",text_seg);


                        }

                    }

                }
                else {    // 'la' 가 아닌 경우
                    fputs(temp, text_seg);
                    fputs(" ", text_seg);
                    while (temp != NULL) {
                        temp = strtok(NULL, "\n");
                        if (temp != NULL) {

                            fputs(temp, text_seg);
                            fputs("\n", text_seg);
                        }
                    }
                }
            }
        }
        address += BYTES_PER_WORD;
    }

}

/******************************************************
 * Function: main
 *
 * Parameters:
 *  int
 *      argc: the number of argument
 *  char*
 *      argv[]: array of a sting argument
 *
 * Return:
 *  return success exit value
 *
 * Info:
 *  The typical main function in C language.
 *  It reads system arguments from terminal (or commands)
 *  and parse an assembly file(*.s).
 *  Then, it converts a certain instruction into
 *  object code which is basically binary code.
 *
 *******************************************************/

int main(int argc, char* argv[])
{
    FILE *input, *output;
    char *filename;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <*.s>\n", argv[0]);
        fprintf(stderr, "Example: %s sample_input/example?.s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Read the input file */
    input = fopen(argv[1], "r");
    if (input == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /* Create the output file (*.o) */
    filename = strdup(argv[1]); // strdup() is not a standard C library but fairy used a lot.
    if(change_file_ext(filename) == NULL) {
        fprintf(stderr, "'%s' file is not an assembly file.\n", filename);
        exit(EXIT_FAILURE);
    }

    output = fopen(filename, "w");
    if (output == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /******************************************************
     *  Let's complete the below functions!
     *
     *  make_symbol_table(FILE *input)
     *  make_binary_file(FILE *output)
     *  ├── record_text_section(FILE *output)
     *  └── record_data_section(FILE *output)
     *
     *******************************************************/
    make_symbol_table(input);
    make_binary_file(output);

    fclose(input);
    fclose(output);

    return 0;
}
