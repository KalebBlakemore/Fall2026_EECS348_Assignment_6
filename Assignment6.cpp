
/**
 * Program Name: EECS 348 Assignment 6 - Sudoku Solver (Optimized)
 * Description: Object-oriented recursive depth-first search with backtracking.
 *              Optimizations over the baseline:
 *                1. Bitmask constraint tracking (row/col/box) -> O(1) validity checks
 *                2. Most-Constrained-Cell (MRV) selection -> far fewer dead-end branches
 *                3. Early dead-end detection (cell with zero candidates)
 *                4. Enumerates ALL solutions (with an optional cap) instead of only the first
 *                5. Validates puzzle input (bad characters, conflicting givens)
 *              Assignment 6 change: every solution of every puzzle is written to ONE file,
 *              solutions.txt, instead of one file per puzzle.
 * Inputs: puzzle1.txt ... puzzle5.txt (9x9 grids; digits 1-9, '_' for an empty cell)
 * Output: Console progress messages, plus solutions.txt. For each puzzle the file holds:
 *           Puzzle: <puzzle file name>
 *           "No solution found."  OR  every solution grid followed by "Solutions found: n"
 * Collaborators: Ryan Biven (his Assignment 5 submission is the code I modified)
 * Other Code Sources: Anthropic Claude (Claude Sonnet 5.5) wrote the streaming-output change;
 *                     OpenAI ChatGPT was the second GenAI compared in Assignment6GenAI.pdf,
 *                     but none of its code is used here.
 * Author: Kaleb Justice Blakemore, Revised by Ryan Biven
 * Modified: 10/29
 * Additional improvemnts: Produce outputs for solutions as files
 * Revised by: Kaleb Justice Blakemore (Assignment 6)
 * Creation Date: 11/12/2026
 * Revision Date: 11/12/2026
 *
 * Code attribution tags used in the comments below:
 *   [A5]     = Assignment 5 code, left unchanged
 *   [A5-MOD] = Assignment 5 code that I changed for Assignment 6
 *   [AI]     = written by Claude, then reviewed and edited by me
 *   [ME]     = written by me
 */
 
#include <iostream>   // [A5] cout/cerr for console messages
#include <fstream>    // [A5] ifstream to read puzzles, ofstream to write solutions.txt
#include <string>     // [A5] string for file names and error text
#include <bitset>     // [A5] bitset<9>::count() to count the legal digits in a cell
 
using namespace std;  // [A5] lets the code use cout, string, etc. without "std::"
 
class SudokuBoard {
private:
    static const int N = 9;         // [A5] the board is N x N cells
    static const int ALL = 0x1FF;   // [A5] bits 0..8 set -> digits 1..9
 
    int grid[N][N];                 // [A5] board values (0 = empty)
    int rowMask[N];                 // [A5] bit (d-1) set if digit d used in row
    int colMask[N];                 // [A5] ... in column
    int boxMask[N];                 // [A5] ... in 3x3 box
 
    long long solutionCount;        // [A5] solutions found so far
    long long solutionLimit;        // [A5] stop after this many (0 = no limit)
    ostream* solutionOut;           // [A5-MOD] stream that receives each solution as it is found;
                                    //          replaces the old haveFirst/firstSolution fields
 
    // [A5] Box number (0..8) that contains cell (r, c)
    static int boxIndex(int r, int c) { return (r / 3) * 3 + (c / 3); }
 
    // Place / remove a digit and keep the masks in sync
    void place(int r, int c, int d) {              // [A5] put digit d in cell (r, c)
        int bit = 1 << (d - 1);                    // bit that stands for digit d
        grid[r][c] = d;                            // write the digit on the board
        rowMask[r] |= bit;                         // mark d as used in this row
        colMask[c] |= bit;                         // mark d as used in this column
        boxMask[boxIndex(r, c)] |= bit;            // mark d as used in this box
    }
    void removeDigit(int r, int c) {               // [A5] undo place() for cell (r, c)
        int bit = 1 << (grid[r][c] - 1);           // bit of the digit now in the cell
        rowMask[r] &= ~bit;                        // free it in the row
        colMask[c] &= ~bit;                        // free it in the column
        boxMask[boxIndex(r, c)] &= ~bit;           // free it in the box
        grid[r][c] = 0;                            // empty the cell
    }
 
    // Bitmask of digits that can legally go in (r, c)
    int candidates(int r, int c) const {           // [A5] digits not yet used in row, column or box
        return ALL & ~(rowMask[r] | colMask[c] | boxMask[boxIndex(r, c)]);
    }
 
    // Recursive DFS with backtracking. Returns true if search should stop.
    bool search() {                                // [A5-MOD] no longer copies a first solution
        // Pick the empty cell with the fewest candidates (MRV)          [A5]
        int bestR = -1, bestC = -1, bestMask = 0, bestCount = 10;     // 10 is above any real count
        for (int r = 0; r < N && bestCount > 1; r++) {                // stop early on a forced cell
            for (int c = 0; c < N; c++) {
                if (grid[r][c] != 0) continue;                        // skip filled cells
                int mask = candidates(r, c);                          // legal digits for this cell
                int cnt = (int)bitset<9>(mask).count();               // how many there are
                if (cnt < bestCount) {                                // most constrained so far
                    bestCount = cnt; bestR = r; bestC = c; bestMask = mask;
                    if (cnt <= 1) break;    // 0 = dead end, 1 = forced move
                }
            }
        }
 
        // No empty cells left: a complete solution
        if (bestR == -1) {
            solutionCount++;                       // [A5] count this solution
            if (solutionOut != nullptr) {          // [A5-MOD] write it straight to solutions.txt
                printGrid(*solutionOut, grid);     // [AI] the 9 rows of the solution
                *solutionOut << "\n";              // [AI] blank line after each grid
            }
            return solutionLimit != 0 && solutionCount >= solutionLimit;   // [A5] true = cap reached
        }
 
        if (bestCount == 0) return false;   // [A5] dead end -> backtrack immediately
 
        // Try each candidate digit in the chosen cell
        for (int d = 1; d <= 9; d++) {
            if (!(bestMask & (1 << (d - 1)))) continue;   // [A5] digit d is not legal here
            place(bestR, bestC, d);                       // [A5] try it
            bool stopSearch = search();                   // [A5-MOD] remember whether the cap was reached
            removeDigit(bestR, bestC);                    // [A5-MOD] always undo, so the board is restored
            if (stopSearch) return true;                  // [A5-MOD] limit reached, unwind
        }
        return false;                                     // [A5] every digit tried
    }
 
    static void printGrid(ostream& out, const int g[N][N]) {   // [A5-MOD] same output format, now one write per grid
        char buf[N * (2 * N + 1)];                             // [AI] 9 rows x (9 cells x 2 chars + newline) = 171 chars
        int k = 0;                                             // [AI] next free position in buf
        for (int i = 0; i < N; i++) {                          // each row
            for (int j = 0; j < N; j++) {                      // each column
                buf[k++] = (g[i][j] == 0) ? '_' : (char)('0' + g[i][j]);   // empty cell is '_', else the digit
                buf[k++] = ' ';                                // every cell is followed by a space
            }
            buf[k++] = '\n';                                   // end of row
        }
        out.write(buf, k);                                     // [AI] one stream call per grid instead of ~81 insertions
    }
 
public:
    SudokuBoard() : solutionCount(0), solutionLimit(0), solutionOut(nullptr) {   // [A5-MOD] no output stream yet
        for (int i = 0; i < N; i++) {
            rowMask[i] = colMask[i] = boxMask[i] = 0;         // nothing used yet
            for (int j = 0; j < N; j++) grid[i][j] = 0;       // every cell empty
        }
    }
 
    // Loads puzzle; returns false (with message in err) on I/O error or invalid puzzle
    bool loadPuzzle(const string& filename, string& err) {    // [A5]
        ifstream file(filename);                              // open the puzzle file
        if (!file.is_open()) { err = "Could not open file " + filename; return false; }
 
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                char ch;                                      // one cell symbol
                if (!(file >> ch)) { err = "File has fewer than 81 cells"; return false; }
                if (ch == '_' || ch == '0' || ch == '.') continue;     // these mean "empty"
                if (ch < '1' || ch > '9') { err = string("Invalid character '") + ch + "'"; return false; }
                int d = ch - '0';                             // symbol -> number
                if (!(candidates(i, j) & (1 << (d - 1)))) {   // digit already used in row/column/box?
                    err = "Conflicting given digits (puzzle is invalid)";
                    return false;
                }
                place(i, j, d);                               // accept the given digit
            }
        }
        return true;                                          // all 81 cells read and consistent
    }
 
    void printBoard() const { printGrid(cout, grid); }        // [A5] show the starting grid on the console
 
    // [A5-MOD] Find solutions, streaming each one to 'out'; limit = max to find (0 = find all).
    // Returns number found.
    long long solve(ostream& out, long long limit = 0) {
        solutionCount = 0;        // reset the counter
        solutionLimit = limit;    // remember the cap
        solutionOut = &out;       // send solutions to this stream
        search();                 // run the backtracking search
        solutionOut = nullptr;    // detach the stream so no stale pointer is kept
        return solutionCount;     // how many solutions were found
    }
};
 
int main() {
    const string puzzleFiles[5] = {"puzzle1.txt", "puzzle2.txt", "puzzle3.txt",
                                   "puzzle4.txt", "puzzle5.txt"};   // [A5] input files
    const long long MAX_SOLUTIONS = 10000;  // [A5] cap on solutions to enumerate (0 = unlimited; unsafe for sparse puzzles)
    const string outFileName = "solutions.txt";                     // [ME] the single output file
 
    ofstream out(outFileName);                                      // [ME] open once for all five puzzles
    if (!out.is_open()) {                                           // [ME] cannot continue without the file
        cerr << "Error: could not open " << outFileName << " for writing\n";
        return 1;
    }
 
    for (int i = 0; i < 5; i++) {                                   // [A5] one pass per puzzle
        cout << "Puzzle File: " << puzzleFiles[i] << "\n";          // [A5] console heading
        cout << "-----------------------------------\n";
        out << "Puzzle: " << puzzleFiles[i] << "\n";                // [ME] required "Puzzle: x" line
 
        SudokuBoard board;                                          // [A5] fresh board for each puzzle
        string err;                                                 // [A5] receives load errors
        if (!board.loadPuzzle(puzzleFiles[i], err)) {               // [A5-MOD] error goes to console and file
            cout << "Error: " << err << "\n";
            out << "Error: " << err << "\n\n";
            cout << "\n===================================\n\n";
            continue;                                               // skip solving, go to the next puzzle
        }
 
        cout << "Initial Puzzle:\n";                                // [A5] show the starting grid
        board.printBoard();
        cout << "\n";
 
        long long count = board.solve(out, MAX_SOLUTIONS);          // [A5-MOD] solutions stream into solutions.txt
        if (count > 0) {                                            // [A5-MOD] at least one solution
            out << "Solutions found: " << count << "\n\n";          // [ME] required summary line
            cout << "Solutions found: " << count << "\n";
            if (MAX_SOLUTIONS != 0 && count >= MAX_SOLUTIONS)       // [ME] console-only note, so the
                cout << "(search stopped at the " << MAX_SOLUTIONS  //      file keeps only the required lines
                     << "-solution cap)\n";
        } else {                                                    // [A5-MOD] no solutions
            out << "No solution found.\n\n";                        // [ME] required wording, with the period
            cout << "No solution found.\n";
        }
        cout << "Results written to " << outFileName << "\n";       // [ME]
        cout << "\n===================================\n\n";       // [A5]
    }
 
    out.close();                                                    // [ME] flush and close the output file
    if (!out) {                                                     // [ME] true if any write failed
        cerr << "Error: a problem occurred while writing " << outFileName << "\n";
        return 1;
    }
    cout << "All puzzles have been processed. Results are in " << outFileName << ".\n";   // [ME]
    return 0;
}