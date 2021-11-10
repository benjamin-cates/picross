#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <emscripten.h>
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using namespace std;
typedef enum Cell : char {
    unsure = 3, broken = 1, painted = 2
} Cell;
typedef struct Hint {
    unsigned char total;
    unsigned char pieceCount;
} Hint;
class Puzzle {
public:
    string name;
    int dimension;
    int* size = NULL;
    Cell* shape = NULL;
    Hint* hints = NULL;
private:
    int maxFaceSize;
    int shapeSize;
public:
    Puzzle(string str) {
        string strs[10];
        int currentIndex = 0;
        int i = 0;
        //Split by tildes
        int lastTilde = -1;
        int x = 0;
        for(int i = 0;i < str.size() + 1;i++) {
            if(str[i] == '~' || str[i] == 0) {
                strs[x] = str.substr(lastTilde + 1, i - lastTilde - 1);
                lastTilde = i;
                x++;
            }
        }
        //Set name
        name = strs[0];
        //Dimension and size
        dimension = atoi(strs[1].c_str());
        free(size);
        size = (int*)calloc(dimension, sizeof(int));
        for(int i = 0;i < strs[2].size();i++) size[i] = strs[2][i] - 'A';
        cout << size[0] << endl;
        //Reconstruct shape size
        int minimumRowLength = 10000;
        shapeSize = 1;
        for(int i = 0;i < dimension;i++) {
            shapeSize *= size[i];
            if(size[i] < minimumRowLength) minimumRowLength = size[i];
        }
        maxFaceSize = shapeSize / minimumRowLength;
        //Copy shape
        free(shape);
        shape = (Cell*)calloc(strs[3].size(), sizeof(Cell));
        for(int i = 0;i < strs[3].size();i++) {
            if(strs[3][i] == ' ') shape[i] = broken;
            else if(strs[3][i] == '-') shape[i] = unsure;
            else if(strs[3][i] == '+') shape[i] = painted;
        }
        //Copy hints
        free(hints);
        hints = (Hint*)calloc(dimension * maxFaceSize, sizeof(Hint));
        for(int i = 0;i < strs[4].size();i += 2) {
            hints[i].total = strs[4][i] - 'A';
            hints[i].pieceCount = strs[4][i + 1] - 'A';
        }
    }
    Puzzle(int dim, int* siz, string nam) {
        dimension = dim;
        free(size);
        size = (int*)calloc(dim, sizeof(int));
        memcpy(size, siz, dim * sizeof(int));
        name = nam;
        int minimumRowLength = 10000;
        shapeSize = 1;
        for(int i = 0;i < dim;i++) {
            shapeSize *= size[i];
            if(size[i] < minimumRowLength) size[i] = minimumRowLength;
        }
        maxFaceSize = shapeSize / minimumRowLength;
        free(shape);
        shape = (Cell*)calloc(shapeSize, sizeof(Cell));
    }
    void solve() {

    }
    void generateHints() {
        free(hints);
        hints = (Hint*)calloc(maxFaceSize * dimension, sizeof(Hint));
        for(int i = 0;i < shapeSize;i++) {
            for(int dim = 0;dim < dimension;dim++) {
                int rowPos = getRowPosition(i, dim);
                //To prevent duplicate calculations
                if(hints[rowPos + dim * maxFaceSize].pieceCount != 0) continue;
                int spacing = 1;
                for(int x = 0;x < dimension;x++) spacing *= size[x];
                //Set i to first cell in the row
                i = ((i / spacing) / size[dim]) * spacing * size[dim] + i % spacing;
                int total = 0, pieces = 0, prev = broken;
                for(int x = 0;x < size[dim];x++) {
                    int cell = i + x * spacing;
                    if(shape[cell] == painted) {
                        total++;
                        if(prev == broken) pieces++;
                    }
                    prev = shape[cell];
                }
                hints[rowPos + dim * maxFaceSize].pieceCount = pieces;
                hints[rowPos + dim * maxFaceSize].total = total;
            }
        }
    }
    int getRowPosition(int pos, int direction) {
        //Example:
        // size = {5,2,3,4}
        // pos = 48, direction = 2
        // belowSize = 10
        // below = 8
        // above = 10
        int belowSize = 1;
        for(int i = 0;i < direction;i++) belowSize *= size[i];
        int below = pos % belowSize;
        int above = (pos / belowSize / size[direction]) * belowSize;
        return below + above;
    }
    void posToPositionArray(int pos, int* out) {
        for(int i = 0;i < dimension;i++) {
            out[i] = pos % size[i];
            pos /= size[i];
        }

    }
    string toString() {
        // NAME~DIMENSION~SIZE~SHAPE~HINTS
        string out = name + "~" + std::to_string(dimension) + "~";
        for(int i = 0;i < dimension;i++) {
            cout << size[i] << endl;
            out += string(1, 'A' + size[i]);
        }
        //Shape
        out += "~";
        for(int i = 0;i < shapeSize;i++) {
            if(shape[i] == broken) out += " ";
            else if(shape[i] == unsure) out += "-";
            else if(shape[i] == painted) out += "+";
        }
        //Hints
        out += "~";
        for(int i = 0;i < maxFaceSize * dimension;i++) {
            out += string(1, char('A' + hints[i].total)) + string(1, char('A' + hints[i].pieceCount));
        }
        return out;
    }
    int collapsePosition(int* position) {
        int sliceSize = 1;
        int out = 0;
        for(int i = 0;i < dimension;i++) {
            out += position[i] * sliceSize;
            sliceSize *= size[i];
        }
        return out;
    }
    Cell getCell(int* position) {
        return shape[collapsePosition(position)];
    }
    Hint getHint(int* position, int direction) {
        int s = 1;
        int pos = 0;
        for(int i = 0;i < dimension;i++) {
            if(i == direction) continue;
            pos += position[i] * s;
            s *= size[i];
        }
        Hint out = hints[maxFaceSize * direction + pos];
        return out;
    }
    void setCell(int* position, Cell set) {
        shape[collapsePosition(position)] = set;
    }
    void getRow(int* position, int dimension, Cell* out) {
        int rowLength = size[dimension];
        int spacing = 1;
        for(int i = 0;i < dimension;i++) spacing *= size[i];
        position[dimension] = 0;
        int firstCell = collapsePosition(position);
        for(int i = 0;i < rowLength;i++) {
            out[i] = shape[firstCell + spacing * i];
        }
    }
    void setRow(int* position, int dimension, Cell* cells) {
        int rowLength = size[dimension];
        int spacing = 1;
        for(int i = 0;i < dimension;i++) spacing *= size[i];
        position[dimension] = 0;
        int firstCell = collapsePosition(position);
        for(int i = 0;i < rowLength;i++) {
            shape[firstCell + spacing * i] = cells[i];
        }
    }
};
Puzzle* puz = new Puzzle("A~1~B~ ~AB");
typedef struct SpacingPossiblity {
    unsigned char space[8];
    void print() {
        for(int i = 0;i < 8;i++) cout << int(space[i]) << " ";
        cout << endl;
    }
} SpacingPossibility;
SpacingPossibility NULLPOSSIB = { {0,0,0,0,0,0,0,0} };

vector<SpacingPossibility> spacePossibilities(int total, int pieces, bool zeroStart, bool zeroEnd) {
    vector<SpacingPossibility> out(0);
    if(pieces == 1) {
        out.push_back(NULLPOSSIB);
        out[0].space[0] = total;
        return out;
    }
    else if(pieces == 2) {
        int x = 0;
        for(int i = zeroStart ? 0 : 1;i <= total - (zeroEnd ? 0 : 1);i++) {
            out.push_back(NULLPOSSIB);
            out[x].space[0] = i;
            out[x].space[1] = total - i;
            x++;
        }
        return out;
    }
    else {
        int margin = total - pieces + (zeroEnd ? 1 : 0) + 1;
        for(int i = zeroStart ? 0 : 1;i <= margin;i++) {
            vector<SpacingPossibility> possib = spacePossibilities(total - i, pieces - 1, false, zeroEnd);
            int size = out.size();
            for(int x = 0;x < possib.size();x++) {
                out.push_back(NULLPOSSIB);
                out[size + x].space[0] = i;
                memcpy(out[size + x].space + 1, possib[x].space, 7);
            }
        }
    }
    return out;
}
void orCells(Cell* dest, Cell* in, int len) {
    for(int i = 0;i < len;i++) dest[i] = Cell(int(in[i]) | int(dest[i]));
}
bool possibilityMatchesRow(Cell* row, Cell* possibility, int len) {
    for(int i = 0;i < len;i++) {
        if((int(row[i]) | int(possibility[i])) != int(row[i])) return false;
    }
    return true;
}
void solveRow(int total, int pieces, int rowLength, Cell* row, Cell* out) {
    //Generate possibilities for broken run
    vector<SpacingPossibility> bkn = spacePossibilities(rowLength - total, pieces + 1, true, true);
    //Generate possibilities for colored runs
    vector<SpacingPossibility> crd = spacePossibilities(total, pieces, false, false);
    //Go through all possibilities
    for(int b = 0;b < bkn.size();b++) {
        //Check if either all unsure or matches current row to break
        if(bkn[b].space[0] != 0 && row[0] == painted) continue;
        for(int c = 0;c < crd.size();c++) {
            Cell possib[rowLength];
            memset(possib, 0, rowLength);
            int pos = 0;
            int countTotal = bkn[b].space[0];
            memset(possib, broken, countTotal);
            for(int j = 0;j < pieces;j++) {
                int count = crd[c].space[j];
                memset(possib + countTotal, painted, count);
                countTotal += count;
                int gap = bkn[b].space[j + 1];
                memset(possib + countTotal, broken, gap);
                countTotal += gap;
            }

            if(possibilityMatchesRow(row, possib, rowLength)) {
                orCells(out, possib, rowLength);
            }
        }
    }
}
void stringToRow(string str, Cell* row, int len) {
    memset(row, 3, len);
    for(int i = 0;i < str.size();i++) {
        if(str[i] == ' ') row[i] = broken;
        if(str[i] == '+') row[i] = painted;
    }
};
string rowToString(Cell* row, int len) {
    string out(len, ' ');
    char types[4] = { 'E', ' ', '+', '?' };
    for(int i = 0;i < len;i++) {
        out[i] = types[int(row[i])];
    }
    out[len] = 0;
    return out;
}
string solveRowJS(int total, int pieces, int rowLength, string row) {
    Cell rowCells[rowLength];
    Cell out[rowLength];
    stringToRow(row, rowCells, rowLength);
    memset(out, 0, rowLength);
    solveRow(total, pieces, rowLength, rowCells, out);
    return rowToString(out, rowLength);
}
string getPuzzle() {
    return puz->toString();
}
void setPuzzle(string str) {
    puz = new Puzzle(str);
}
void doHints() {
    puz->generateHints();
}
void solveCurrentPuzzle() {
    puz->solve();
}
EMSCRIPTEN_BINDINGS(a) {
    emscripten::function("solveRow", &solveRowJS);
    emscripten::function("getPuzzle", &getPuzzle);
    emscripten::function("setPuzzle", &setPuzzle);
    emscripten::function("generateHints", &doHints);
    emscripten::function("solve", &solveCurrentPuzzle);
}
int main() {
    return 0;
}