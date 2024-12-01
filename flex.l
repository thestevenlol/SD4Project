%option noyywrap
%{
int line_num = 1;
%}

identifier [a-zA-Z_][a-zA-Z0-9_]*
whitespace [ \t\n]+

%%

(int|float|char|double|bool|void|long|short)[ \t]+input[ \t]*=[ \t]*([^;]+); { 
    printf("Line %d: 'input' declared and initialised to: %s\n", line_num, yytext); 
}

input[ \t]*=[ \t]*([^;]+); { 
    printf("Line %d: 'input' assigned to: %s\n", line_num, yytext); 
}

input[ \t]*!=[ \t]*[^ \t\)\}&;]+ { 
    printf("Line %d: 'input' assigned to: %s\n", line_num, yytext); 
}

input[ \t]*[+\-*/][^;]+; { 
    printf("Line %d: 'input' used in arithmetic: %s\n", line_num, yytext);
}

input[ \t]*\(.*\); { 
    printf("Line %d: 'input' used in function call: %s\n", line_num, yytext);
}

\n { line_num++; }

. { }

%%

int main(int argc, char *argv[]) {
    yylex(); 
    return 0;
}
