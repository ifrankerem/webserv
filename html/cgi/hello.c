#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const char *method = getenv("REQUEST_METHOD");
    const char *query = getenv("QUERY_STRING");

    printf("Content-Type: text/html\r\n\r\n");
    printf("<html><body>\n");
    printf("<h1>Merhaba, C CGI!</h1>\n");
    printf("<p>Method: %s</p>\n", method ? method : "-");
    printf("<p>Query: %s</p>\n", query ? query : "-");
    printf("</body></html>\n");
    return 0;
}
