#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/*
 command for compiling:
 gcc -Wall -o maxTweeter maxTweeter.c
 command for running:
 ./maxTweeter cl-tweets-short.csv
 */

/*
 results on our runs:
 non-empty slots in hash table: 6228
 */

//STRUCTS
struct Slot
{
    char* name;//NULL if empty
    int tweetCount;//-1 if empty
};
//initial values
#define IV_name ( NULL )
#define IV_tweetCount ( -1 )

//FUNCTIONS
void ASSERT_lineIsValid( char* line );
void establishExpectations( char* headerLine );
void processValidLine( char* line, struct Slot* hashtable );
void invalidInputMessage( const char* messageIfDebugging );
char* getfield(char* line, int num);
void assignSlot( struct Slot* s, char* name );
void initializeSlot( struct Slot* s );
bool isEmpty( struct Slot s );
char* getName( struct Slot s );
int getNameLength( struct Slot s );
int getTweetCount( struct Slot s );
void incTweetCount( char* name, struct Slot* hashtable );
unsigned long hashCode( unsigned char* str );
struct Slot* findRightfulSlot( char* name, struct Slot* hashtable );
unsigned long hash( unsigned long key, unsigned long attempts );
int compareSlots( struct Slot s1, struct Slot s2);
void heapify( struct Slot* hashtable, int size, int i );
void heapSort( struct Slot* hashtable, int size );


//ERROR RETURN CODES
#define PROGRAM_ERROR (-1)
#define USER_ERROR ( PROGRAM_ERROR - 1 )


//GLOBAL CONSTANTS
const int MAX_lineLength = 374;
//longest row from hw3
//by character count
const int MAX_rowCount = 20000;
//20,000 rows + 1 header
#define HASHTABLE_SIZE ( 40009 )
#define BUFFER_SIZE ( MAX_lineLength + 2 )
//needs to be longer than MAX_lineLength
//since user can go over lineLength limit
const char COLUMN_DELIMITER = ',';


//GLOBAL VARIABLES
int EXPECTED_columnCount = -1;
//number of columns in header
//set by establishExpectations
int NAME_COLUMN = -1;
#define MAX_columnLength ( MAX_lineLength - (EXPECTED_columnCount-1) )


bool debugging = false;
//set to false before submittting to prevent debugging output
//set to true to allow custom debugging messages

int main( int argc, char *argv[] )
{
    if ( argc != 2 )//incorrect number of arguments
    {
        return -1;
    }
    
    struct Slot hashtable[ HASHTABLE_SIZE ];
    
    //every slot should be empty initially
    for ( int i = 0; i < HASHTABLE_SIZE; i++ )
    {
        initializeSlot( &hashtable[i] );
    }
    
    //read in the file in command line
    FILE *file = fopen(argv[1], "r");
    
    //notify and exit if file is invalid
    if ( !file )
    {
        fprintf(stderr, "Can't open file.\n");
        exit( USER_ERROR );
    }
    
    char line[ BUFFER_SIZE ];//buffer for one line
        //will be used to repeatedly read in lines
        //one at a time from a huge file
    
    /*
     fgets returns false when:
        end-of-file hit && no characters read in yet
     fgets copies one csv row from file to line
     BUFFER_SIZE limits number of characters taken in
     */
    
    
    int lineCountdown = MAX_rowCount;
        //to enforce total row limit
    
    if ( fgets(line,BUFFER_SIZE,file) )
    {
        lineCountdown--;
        
        ASSERT_lineIsValid( line );
        //make sure this line is secure
        
        establishExpectations( line );
        //expectations come from header
        //e.g. how many columns to expect
        
        while ( fgets(line,BUFFER_SIZE,file) )
        {
            lineCountdown--;
            
            if ( lineCountdown == 0 )
            {
                char message[50];
                sprintf(
                        message
                        , "exceeded maximum row count\n"
                        );
                invalidInputMessage( message );
                exit( USER_ERROR );
            }
            
            if ( line[0] == '\n' || line[0] == '\r' )//if line is empty
            {
                continue;//continue to next line
            }
            
            ASSERT_lineIsValid( line );
            //make sure line is secure
            
            processValidLine( line, hashtable );
            //everything we want to do to each row
            //is carried out by this function
        }
        
        
        if ( lineCountdown+1 == MAX_rowCount )
            //we only read in the header
            //we never entered the while-loop
        {
            char message[50];
            sprintf(
                    message
                    , "empty file\n"
                    );
            invalidInputMessage( message );
            exit( USER_ERROR );
        }
        
        heapSort( hashtable, HASHTABLE_SIZE );
        
        int hashTableEntries = 0;
        for ( int i = 0; i < HASHTABLE_SIZE; i++ )
        {
            if ( !isEmpty(hashtable[i]) )
            {
                hashTableEntries++;
            }
            
            // if ( debugging )
            // {
            //     printf( "%d %s : %d\n", i, hashtable[i].name, hashtable[i].tweetCount );
            // }
        }
        
        // if ( debugging )
        // {
        //     printf( "NAME_COLUMN: %d\n", NAME_COLUMN );
        //     printf( "non-empty slots in hash table: %d\n", hashTableEntries );
        // }
        
        // <tweeter>: <count of tweets>
        for ( int i = HASHTABLE_SIZE - 1
             ; i >= HASHTABLE_SIZE - 10 && !isEmpty(hashtable[i])
             //!isEmpty is for when there are <10 tweets
             ; i-- )
        {
            printf("%s: %d\n", hashtable[i].name, hashtable[i].tweetCount );
        }
    }
    else
    {
        char message[50];
        sprintf(
                message
                , "empty file\n"
                );
        invalidInputMessage( message );
        exit( USER_ERROR );
    }
    
    for ( int i = 0; i < HASHTABLE_SIZE; i++ )
    {
        if ( !isEmpty( hashtable[i] ) )
        {
            free( hashtable[i].name );
        }
    }
    
    fclose(file);
    return 0;
}

/*
 processes one line from the csv
 ASSUMPTIONS:
    line is valid
 */
void processValidLine( char* line, struct Slot* hashtable )
{
    char* name = getfield(line,NAME_COLUMN);
    
    if ( !name )//if getfield can't get the name
    {
        //this would only happen if ASSERT_lineIsValid
        //didn't do its job
        
        char message[50];
        sprintf(
                message
                , "no name in this line\n"
                );
        invalidInputMessage( message );
        exit( PROGRAM_ERROR );
    }
    
    /*
     getfield does not remove '\n'.
     The following code will remove a dangling '\n' if necessary.
     */
    unsigned long name_length = strlen( name );
    if ( name[name_length-1] == '\n' || name[name_length-1] == '\r' )//strlen stops at '\0', not '\n'
    {
        name_length--;//insert '\0' in name_copy below
    }
    
    char* name_copy = malloc( (name_length+1)*sizeof(char) );
    //this needs to be freed when deleting the hashtable
    
    strcpy( name_copy, name );
    name_copy[ name_length ] = '\0';//insert '\0'
    
    incTweetCount( name_copy, hashtable );
}

/*
 will terminate program if:
 first non-printable character (eof?) is after MAX_lineLength
 number of columns in line != number of expected columns
 (unless this is the header)
 ASSUMPTIONS:
    line has length BUFFER_SIZE
 */
void ASSERT_lineIsValid( char* line )
{
    int i, columnCount;
    bool betweenQuotations = false;
    
    
    for ( i = 0, columnCount = 1
         ; i < BUFFER_SIZE
         && line[i] != '\n' && line[i] != '\r' && line[i] != '\0'//not the end of line
         ; i++ )
    {
        //loop stops at index of
        //end of line
        
        if ( line[i] == COLUMN_DELIMITER && !betweenQuotations )
            //ignore commas that are part of a content string
        {
            columnCount++;
        }
        else if ( line[i] == '\"' )
        {
            betweenQuotations = !betweenQuotations;
            //could be opening or closing parentheses
        }
        
        if ( line[i] == COLUMN_DELIMITER && betweenQuotations )
        {
            char message[50];
            sprintf(
                    message
                    , "commas aren't allowed between quotations"
                    );
            invalidInputMessage( message );
            exit( USER_ERROR );
        }
    }
    
    /*
    i--;//remove '\n' or '\0' from loop
    i++;//we want i to represent character count, not last index
    //these even out
     */
    
    if ( i <= 0 )//line is empty
    {
        char message[50];
        sprintf(
                message
                , "empty line\n"
                );
        invalidInputMessage( message );
        exit( USER_ERROR );
    }
    else if ( MAX_lineLength < i )//line was too long
    {
        char message[50];
        sprintf(
                message
                , "line length exceeds limit: %d < %d\n"
                , MAX_lineLength , i
                );
        invalidInputMessage( message );
        exit( USER_ERROR );
    }
    
    /*
     The number of columns in all lines should
     match the header's number of columns.
     This number is saved in EXPECTED_columnCount.
     EXPECTED_columnCount is set to -1
     before the header is read.
     Therefore, we'll only check if this line's number of
     columns matches the header's
     if 0 < EXPECTED_columnCount
     */
    if ( 0 < EXPECTED_columnCount//header has been read
        && columnCount != EXPECTED_columnCount//doesn't match header
        )
    {
        
        char message[50];
        sprintf(
                message
                , "incorrect number of columns: %d != %d\n"
                , columnCount, EXPECTED_columnCount
                );
        invalidInputMessage( message );
        exit( USER_ERROR );
    }
}

/*
 initializes:
 NAME_COLUMN = column index of "name"
 EXPECTED_columnCount = number of columns in header
 ASSUMPTIONS:
 headerLine is a valid line
 headerLine has length BUFFER_SIZE
 INVALID INPUT if:
 multiple name columns detected
 */
void establishExpectations( char* headerLine )
{
    int currentColumn = 1;
    bool matchPossible = true;
    char name[] = "\"name\"";//name column title
    unsigned long nameLength = strlen(name);
    int n = 0;//used to find "name" column
    
    bool stopLoop = false;
    for ( int i = 0; i < BUFFER_SIZE && !stopLoop; i++ )
    {
        if ( headerLine[i] == COLUMN_DELIMITER//hit delimiter
            || headerLine[i] == '\n' || headerLine[i] == '\r'//hit endline
            || headerLine[i] == '\0' )//hit end of line
        {
            if ( 0 <= NAME_COLUMN//match found in previous column
                && matchPossible && n == nameLength )//match in this column
                //multiple "name" columns
            {
                char message[50];
                sprintf(
                        message
                        , "two name columns: %d, %d\n"
                        , NAME_COLUMN , currentColumn
                        );
                invalidInputMessage( message );
                exit( USER_ERROR );
            }
            else if ( matchPossible && n == nameLength )//match
                //match in this column
            {
                NAME_COLUMN = currentColumn;
            }
            
            if ( headerLine[i] != COLUMN_DELIMITER )
                //we finished the line
                //we hit one of the end-of-line characters
            {
                stopLoop = true;
                break;
            }
            else//we hit the delimiter
                //move to next column
            {
                //reset for next columns
                n = 0;
                matchPossible = true;
                currentColumn++;
            }
        }
        else if ( isspace(headerLine[i]) )
            //hit a whitespace (except newline)
        {
            if ( n == 0 )//before match
            {
                continue;
            }
            else if ( n == nameLength )//after match
            {
                continue;
            }
            else//during match
            {
                matchPossible = false;//end search for this match
            }
        }
        else//hit printable or other (apparently emojis aren't detected by isprint)
        {
            if ( n == 0 )//before match
            {
                if ( matchPossible && name[n] == headerLine[i] )
                {
                    n++;//start match
                }
                else// name[n] != headerLine[i]
                {
                    matchPossible = false;//end search for match
                }
            }
            else if ( n == nameLength )//after match
            {
                matchPossible = false;//end search for this match
            }
            else//during match
            {
                if ( matchPossible && name[n] == headerLine[i] )
                {
                    n++;//continue match
                }
                else// name[n] != headerLine[i]
                {
                    matchPossible = false;//end search for match
                }
            }
        }
    }
    EXPECTED_columnCount = currentColumn;//number of header columns
    
    if ( NAME_COLUMN < 0 )
    {
        char message[50];
        sprintf(
                message
                , "no name column\n"
                );
        invalidInputMessage( message );
        exit( USER_ERROR );
    }
}

char* getfield( char* line, int num )
{
    char* tok;
    for ( tok = strtok(line, ",")
         ; tok && *tok
         ; tok = strtok(NULL, ",\n") )
    {
        if ( ! --num )
        {
            return tok;
        }
    }
    return NULL;
}

void invalidInputMessage( const char* messageIfDebugging )
{
    if ( debugging )
    {
        printf( "%s", messageIfDebugging );
    }
    else
    {
        printf("Invalid Input Format");
    }
}

/*
 ==================================================
 HASH TABLE CODE
 ==================================================
 */


void initializeSlot( struct Slot* s )
{
    (*s).name = IV_name;
    (*s).tweetCount = IV_tweetCount;
}

/*
 Will be used to make sure
 we don't destroy a filled slot
 */
void assignSlot( struct Slot* s, char* name )
{
    if ( !isEmpty(*s) )
    {
        char message[50];
        sprintf(
                message
                , "attempt to replace full slot\n"
                );
        invalidInputMessage( message );
        exit( PROGRAM_ERROR );
    }
    else if ( *name == '\0' )
    {
        
    }
    
    (*s).name = name;
    (*s).tweetCount = 1;
}

bool isEmpty( struct Slot s )
{
    return s.name == IV_name;
}

char* getName( struct Slot s )
{
    return s.name;
}

int getTweetCount( struct Slot s )
{
    return s.tweetCount;
}

/*
 will either:
 increment existing slot
 create slot for name,
 then set it to 1
 */
void incTweetCount( char* name, struct Slot* hashtable )
{
    struct Slot* s = findRightfulSlot(name, hashtable);
    if ( isEmpty(*s) )
    {
        assignSlot(s,name);
    }
    else
    {
        free( name );
        (*s).tweetCount++;
    }
}
/*
 hashCode for a string
 will be the sum of its characters' ascii values
 hashCode required for hashing
 using strings as keys
 */
//hash function by dan bernstein at http://www.cse.yorku.ca/~oz/hash.html helps reduce collisions significantly
unsigned long hashCode( unsigned char* str )
{
    unsigned long hash = 5381;
    int c;
    
    while ( (c = *str++) )
    {
        hash = ( (hash<<5) + hash ) + c;
    }
    
    return hash;
}

/*
 will return **either**:
 slot where name is
 empty slot where name should be
 */
struct Slot* findRightfulSlot( char* name, struct Slot* hashtable )
{
    
    unsigned long key = hashCode( (unsigned char*)name );
    
    unsigned long i;
    int attempts = 0;
    
    //continue rehashing until conditions met
    do
    {
        i = hash( key, attempts++ );
        //increment attempts after call
    }
    while(
          !isEmpty( hashtable[i] )
          //not an empty slot
          && ( strcmp( name, getName(hashtable[i]) ) != 0 )
          //not the name we want
          );
    
    return &hashtable[i];
}

/*
 In this context, a string's hashCode
 will be passed as the string
 */
unsigned long hash( unsigned long hashCode, unsigned long attempts )
{
    //TODO define hash
    int i;
    
    if ( attempts == 0 )
    {
        i = hashCode % HASHTABLE_SIZE;
    }
    else
    {
        i = ( hashCode + (attempts*attempts) ) % HASHTABLE_SIZE;
    }
    
    return i;
}

int compareSlots( struct Slot s1, struct Slot s2)
{
    if(s1.tweetCount > s2.tweetCount)
    {
        return 1;
    }
    else if (s2.tweetCount > s1.tweetCount)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

//heapsort adapted from: https://www.geeksforgeeks.org/heap-sort/
void heapify( struct Slot* hashtable, int size, int i )
{
    int l = 2*i + 1;
    int r = 2*i + 2;
    int largest = i;
    
    if( l < size && compareSlots(hashtable[l], hashtable[largest]) > 0 )
    {
        largest = l;
    }
    
    if( r < size && compareSlots(hashtable[r],hashtable[largest]) > 0)
    {
        largest = r;
    }
    
    if ( largest != i )
    {
        struct Slot temp = hashtable[i];
        hashtable[i] = hashtable[largest];
        hashtable[largest] = temp;
        
        heapify(hashtable, size, largest);
    }
    
}
void heapSort( struct Slot* hashtable, int size )
{
    //build the heap
    for( int i = (size/2) - 1; i >= 0; i--)
    {
        heapify( hashtable, size, i );
    }
    
    
    for (int i = size - 1; i >= 0; i-- )
    {
        //swap current root with end
        struct Slot temp = hashtable[i];
        hashtable[i] = hashtable[0];
        hashtable[0] = temp;
        
        //heapify the reduced heap
        heapify(hashtable, i, 0);
    }
}

