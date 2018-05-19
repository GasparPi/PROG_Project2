#include <iostream>
#include <limits>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "Engine.h"

using namespace std;
using namespace BoardTypes;

// Constructor for class Engine
// Clears the screen before doing anything else
Engine::Engine()
    : Dictionary(), Board()
{
    utility.clrscr();
    start();
}

// Displays the title screen (program name and instructions)
void Engine::start()
{
    //Title
    logger.header("greeting");
    logger.log("instructions");

    showMenu();
}

// Displays the menu
void Engine::showMenu()
{
    unsigned char option = '0';

    logger.header("options");
    logger.log("options");

    cin >> option;

    switch (option) {
    case '1': {
        createPuzzle();
        break;
    }
    case '2': {
        //loadPuzzle();
        break;
    }
    case '0': exit(0);

    default:logger.error("InvalidOption");
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

void Engine::createPuzzle()
{
    string inputFileName;
    unsigned int nRows, nCols;

    logger.header("CreatePuzzle");

    cout << "Dictionary file name ? ";
    cin >> inputFileName;

    this->dictionaryFile = inputFileName;

    load(dictionaryFile);

    while (cout << "Board size (lines columns) ? " && !(cin >> nRows >> nCols)) {
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            nRows = 0;
            nCols = 0;

            logger.log("InvalidValues");
        }
    }

    makeBoard(nRows, nCols);
    writeBoard(cout, 1);

    addWordDialogue();
}

// Dialogue to insert the COORDINATE where the word will be inserted
void Engine::addWordDialogue()
{
    COORDINATE initialCoord;
    char direction;
    string option;
    bool anotherChange = true;

    while (anotherChange) {
        cout << "Position (LCD / CTRL-Z = STOP) ?: ";

        cin >> initialCoord.first >> initialCoord.second >> direction;

        if (cin.fail() && cin.eof()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            writeDialogue();
            exit(0);
        }

        if (coordinateInBoard(initialCoord)) // Check if coordinates are in the board, if they are, show insert word dialogue
        {
            string word;

            cout << "Insert a valid word ('e' for additional options): ";
            cin >> word;

            utility.capitalize(word);
            initialCoord.first = utility.to_upper(initialCoord.first);
            initialCoord.second = utility.to_upper(initialCoord.second);
            direction = utility.to_upper(direction);

            insertWordDialogue(word, initialCoord.first, initialCoord.second, direction);
        }
        else {
            logger.error("InvalidPosition");
        }

        if (!isNotFull()) {

            cout << "Board is full. Do you want to make any changes right now (yes/no) ? ";

            do {
                cin >> option;
                if (option == "yes")
                    addWordDialogue();

                else if (option == "no") {
                    anotherChange = false;
                    writeDialogue();
                }
                else {
                    logger.error("InvalidOption");
                }
            }
            while (option != "yes" && option != "no");
        }
    }
}

void Engine::insertWordDialogue(string word, char verCoord, char horCoord, char direction)
{
    string newWord;
    string coordinate;

    COORDINATE initialCoord(verCoord, horCoord);

    string line = getLine(initialCoord.first, initialCoord.second, direction);

    coordinate.push_back(verCoord);
    coordinate.push_back(utility.to_lower(horCoord));
    coordinate.push_back(direction);

    // Check if the word is in the dictionary
    if (isalpha(word[0]) && isValid(word)) {
        // Check if the word fits in the board
        if (fits(word, verCoord, horCoord, direction)) {
            // Check if the word doesnt match any letters currently on the board
            if (!matches(word, line)) {
                logger.error(word, "NoMatch");
            }
            else if (isCurrentWord(word)) {
                logger.error(word, "AlreadyInserted");
            }
            else {
                currentWords_insert(coordinate, word);
                addWord(word, initialCoord, direction);
            }
        }
        else {
            logger.error(word, "Length");
        }
    }

    else if (word == "E") {
        logger.log("InfoDialogue");

        cin >> newWord;
        utility.capitalize(newWord);

        insertWordDialogue(newWord, verCoord, horCoord, direction);
    }
    else if (word == "-") {
        removeWord(initialCoord, direction, currentWords);
        currentWords_erase(coordinate);
    }

    else if (word == "?") {
        suggestWordsDialogue(verCoord, horCoord, direction);
    }

    else if (word == "?A") {
        cout << "Processing..." << endl;
        suggestAllWordsDialogue();
    }

    else if (word == "R") {
        resetAllDialogue();
    }

    else if (word == "+") {
        checkAutomaticWordsDialogue(verCoord, horCoord, direction);
    }
    else {
        writeBoard(cout, 1);
        logger.error("InvalidWord");
        return;
    }

    writeBoard(cout, 1);
}

void Engine::suggestWordsDialogue(char verCoord, char horCoord, char direction)
{
    string line, word;
    string coordinates;

    line = getLine(verCoord, horCoord, direction);

    horCoord = utility.to_lower(horCoord);
    coordinates.push_back(verCoord);
    coordinates.push_back(verCoord);
    coordinates.push_back(horCoord);
    coordinates.push_back(direction);

    horCoord = utility.to_upper(horCoord);

    storeSuggestions(coordinates, line);

    COORDINATE previousV(static_cast<const char &>(verCoord - 1), horCoord);
    COORDINATE previousH(verCoord, static_cast<const char &>(horCoord - 1));

    if ((direction == 'V' && isalpha(board[previousV])) || (direction == 'H' && isalpha(board[previousH]))) {
        logger.error("ProximityError");
        return;
    }

    if (!suggestions_is_empty()) {
        showSuggestions();

        cout << "Which one of these words do you want to add ? ";
        cin >> word;
        utility.capitalize(word);

        if (fits(word, verCoord, horCoord, direction)) {
            COORDINATE initialCoord(verCoord, utility.to_upper(horCoord));

            currentWords_insert(coordinates, word);
            addWord(word, initialCoord, direction);
        }
        else {
            // TODO: force error
            logger.error("LimiterError");
        }

        clearSuggestions();
    }
    else {
        logger.special(coordinates, "NoValidWords");
    }
}

void Engine::suggestAllWordsDialogue()
{
    string coordinates, word, line;

    auto verCoord =
        static_cast<char>(64); //Character before 'A'. This way 'board.nextCoordinates' updates the coordinates to 'A' and 'a'.
    auto horCoord = static_cast<char>(65 + nCols - 1); //Last character of the row in uppercase

    while (nextCoordinates(verCoord, horCoord)) {
        char direction = 'H';

        for (int i = 0; i < 2; i++) {

            coordinates.push_back(verCoord);
            coordinates.push_back(utility.to_lower(horCoord));
            coordinates.push_back(direction);

            horCoord = utility.to_upper(horCoord);
            line = getLine(verCoord, horCoord, direction);

            storeSuggestions(coordinates, line);

            direction == 'H' ? direction = 'V' : direction = 'H'; //switches between directions
            coordinates.clear();
        }
    }

    showSuggestions();
    clearSuggestions();
}

void Engine::writeDialogue()
{
    string option;
    ofstream outStream;
    static unsigned int boardID = 0;
    ++boardID;

    ostringstream outFileName;
    outFileName << 'b' << setw(3) << setfill('0') << boardID << ".txt";

    string fileName = outFileName.str();

    do {
        cout << "Is the board finished? (yes/no): ";
        cin >> option;

        if (option == "yes") {
            finish();
            break;
        }
        else if (option != "no") {
            logger.error("InvalidOption");
        }
    }
    while (option != "yes" && option != "no");

    cout << "Writing to " << fileName << endl;

    outStream.open(fileName);
    writeBoard(outStream, 0);

    currentWords_send(outStream);
    logger.log("WriteFinished");

    do {
        cout << "Back to menu? (yes/no): ";
        cin >> option;

        if (option == "yes") {
            resetAllDialogue();
            showMenu();
        }
        else if (option == "no") {
            exit(0);
        }
        else {
            logger.error("InvalidOption");
        }
    }
    while (option != "yes" && option != "no");
}

void Engine::resetAllDialogue()
{
    reset();
    currentWords_clear();
    writeBoard(cout, 1);
}

void Engine::checkAutomaticWordsDialogue(char line, char column, char direction)
{
    vector<string> automaticWords;
    string word = getLine(line, column, direction);
    unsigned int pos;
    string coord, option, wordToAdd;

    coord.push_back(line);
    coord.push_back(utility.to_lower(column));
    coord.push_back(direction);

    if (isInitialCoord(coord)) {
        logger.error(coord, "AlreadyOccupied");

        addWordDialogue();
        exit(0);
    }

    //Eliminates the dots on the line, remaining just the word itself
    pos = word.find('.');
    while (pos != string::npos) {
        word.erase(pos);
        pos = word.find('.');
    }

    if (word.empty()) {
        logger.special(coord, "NoWord");

        addWordDialogue();
        exit(0);
    }

    while (!word.empty()) {
        if (isValid(word) && !isCurrentWord(word)) {
            automaticWords.push_back(word);
        }

        word.erase(word.length() - 1);
    }

    if (!automaticWords.empty()) {
        cout << "Valid words automatically formed on the position " << coord << ":" << endl;

        for (const string &autoWord : automaticWords) {
            cout << autoWord << endl;
        }

        cout << "Do you want to add any of these words to the board word list (yes/no) ";

        do {
            cin >> option;
            if (option == "yes") {
                cout << "Word ? ";
                cin >> wordToAdd;

                utility.capitalize(wordToAdd);
                if (binary_search(automaticWords.begin(), automaticWords.end(), wordToAdd)) {
                    currentWords_insert(coord, wordToAdd);
                }

                cout << wordToAdd << " added to board words list." << endl;
            }

            else if (option == "no") {
                addWordDialogue();
                exit(0);
            }
        }
        while (option != "yes" && option != "no");
    }

    else {
        cout << "None valid word was automatically formed on the position " << coord << ":" << endl;
    }
}
