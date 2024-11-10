#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <iomanip>
#include <openssl/sha.h>
#include <numeric>

using namespace std;
using namespace cv;

typedef vector<Vec3b> Row;
typedef vector<int> Key;

int kSS = 128;

/**
 * Create a sha256 hash of an input password.
 * Use the integers as array indices to scramble the initial key.
 */
Key sha256Key(const string &str) {
    // Generate a vector of ints from a sha256 hash.
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    Key out = Key(SHA256_DIGEST_LENGTH);
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        out[i] = (int)hash[i];
    }
    return out;
}

/**
 * Rotate the key integers incrementally.
 */
int bitRot(int n, int bits) {
    // For a given byte, rotate the bits to extend a key.
    int n1 = n >> (8 - bits);
    int n2 = (n << bits) & 0xFF;
    return n1 | n2;
}

/**
 * Extend the sha256 key integers by rotating the bits for each index.
 * Use the resulting numbers to scramble a sequential array of numbers.
 * This will be used as the scramble key.
 */
Key extendKey(Key key) {
    // Output key.
    Key out = {};

    // Sequential array.
    Key vec = Key(kSS);
    for (int i=0; i<vec.size(); i++) vec[i] = i;

    // Based on the key bytes, pop indices out of the sequential array
    // to form the extended scramble key.
    for (int rot=0; rot<8; rot+=2) {
        for (int i=0; i<key.size(); i++) {
            int idx = bitRot(key[i], rot) % vec.size();
            out.push_back(vec[idx]);
            vec.erase(vec.begin() + idx);
        }
    }
    return out;
}

/**
 * Convert an image to a matrix of pixels.
 */
vector<Row> toArray(Mat image) {
    Row row = Row(image.cols, 0);
    vector<Row> array = vector<Row>(image.rows, row);
    for (int y=0; y<image.rows; y++) {
        for (int x=0; x<image.cols; x++) {
            array[y][x] = image.at<Vec3b>(y, x);
        }
    }
    return array;
}

/**
 * Convert a matrix of pixels to an image.
 */
Mat toImage(vector<Row> array, int dtype) {
    Mat out = Mat(array.size(), array[0].size(), dtype);
    for (int y=0; y<array.size(); y++) {
        for (int x=0; x<array[y].size(); x++) {
            out.at<Vec3b>(y, x) = array[y][x];
        }
    }
    return out;
}

template <typename T>

/**
 * Rotate a row of pixels or key integers.
 */
T rotate(T row, int n) {
    // Rotate a single vector by `n` indices.
    if (n < 0) n = row.size() - abs(n);

    T temp = T(row.size());
    for (int i=0; i<row.size(); i++) {
        int offset = (i + n) % row.size();
        temp[i] = row[offset];
    }
    return temp;
}

template <typename T>

/**
 * Rotate a pixel matrix clockwise.
 */
vector<T> rotate_cw(vector<T> array) {
    // Rotate a 2d vector clockwise.
    T row = T(array.size(), 0);
    vector<T> out = vector<T>(array[0].size(), row);
    for (int y=0; y<out.size(); y++) {
        for (int x=0; x<row.size(); x++) {
            int rot_x = row.size() - 1 - x;
            out[y][x] = array[rot_x][y];
        }
    }
    return out;
}

template <typename T>

/**
 * Rotate a pixel matrix counter-clockwise.
 */
vector<T> rotate_ccw(vector<T> array) {
    // Rotate a 2d vector counter-clockwise.
    T row = T(array.size(), 0);
    vector<T> out = vector<T>(array[0].size(), row);
    for (int y=0; y<out.size(); y++) {
        int rot_y = out.size() - 1 - y;
        for (int x=0; x<row.size(); x++) {
            out[y][x] = array[x][rot_y];
        }
    }
    return out;
}

template <typename T>

vector<T> rotate_2d(vector<T> array, bool cw) {
    if (cw) return rotate_cw<T>(array);
    return rotate_ccw<T>(array);
}

template <typename T>

/**
 * Apply a slant to a pixel matrix.
 */
vector<T> slant(vector<T> array, bool do_scramble) {
    float diff = (float)array[0].size() / (float)array.size();
    if (!do_scramble) {
        diff = -diff;
    }

    for (int i=0; i<array.size(); i++) {
        int roll = static_cast<int>(diff * i);
        array[i] = rotate<T>(array[i], roll);
    }

    return array;
}

template <typename T>

/**
 * Scramble a pixel matrix from top to bottom with a scramble key.
 */
vector<T> scramble(vector<T> array, Key key, bool do_scramble) {
    T row = T(array[0].size(), 0);
    vector<T> out = vector<T>(array.size(), row);

    int i = 0;
    for (int idx : key) {
        if (idx >= array.size()) continue;

        if (do_scramble) {
            out[i] = array[idx];
        } else {
            out[idx] = array[i];
        }
        i++;
    }

    return out;
}

template <typename T>

/**
 * Full scramble process.
 */
vector<T> doScramble(vector<T> array, Key key, bool forward) {
    array = scramble<T>(array, key, forward);
    array = slant<T>(array, forward);
    array = scramble<T>(array, key, forward);

    array = rotate_2d<T>(array, true);

    array = scramble<T>(array, key, forward);
    array = slant<T>(array, forward);
    array = scramble<T>(array, key, forward);

    array = rotate_2d<T>(array, false);

    array = scramble<T>(array, key, forward);
    array = slant<T>(array, forward);
    array = scramble<T>(array, key, forward);

    return array;
}

/**
 * Generate a scramble key from an input password.
 */
Key generateKey(const string &str) {
    Key bytes = sha256Key(str);
    Key vect = extendKey(bytes);

    // Create key matrix.
    Key row = Key(kSS, 0);
    vector<Key> array = vector<Key>(kSS, row);
    for (int y=0; y<kSS; y++) {
        for (int x=0; x<kSS; x++) {
            array[y][x] = (y * kSS) + x;
        }
    }

    // Scramble the 2d vector key to adequately shuffle the indices.
    array = doScramble<Key>(array, vect, true);
    
    Key out;
    for(const auto &v: array)
        out.insert(out.end(), v.begin(), v.end());
    return out;
}

bool endsWith(string check, string criteria) {
    if (check.size() >= criteria.size()) {
        string match = check.substr(check.size() - criteria.size(), check.size());
        return match == criteria;
    }
    return false;
}

string getOutputFile(string filename, bool doScramble) {
    string scrambledSuffix = ".scrambled.png";
    string unscrambledSuffix = ".unscrambled.png";

    string prefix = filename;
    int idx = filename.find_last_of(".");
    if (idx > 0) {
        prefix = filename.substr(0, idx);
    }

    if (doScramble) {
        if (endsWith(filename, unscrambledSuffix)) {
            int end = filename.size() - unscrambledSuffix.size();
            prefix = filename.substr(0, end);
        }
        return prefix + scrambledSuffix;

    } else {
        if (endsWith(filename, scrambledSuffix)) {
            int end = filename.size() - scrambledSuffix.size();
            prefix = filename.substr(0, end);
        }
        return prefix + unscrambledSuffix;
    }
}

int main(int argc, char *argv[]) {
    bool scramble = true;
    string filename;
    string password;

    if (argc < 2) {
        cerr << "Not enough arguments" << endl;
        exit(1);
    }

    for (int i=1; i<argc; i++) {
        string arg = argv[i];
        if (arg == "--help") {
            cout << "Usage scrambler METHOD FILENAME [PASSWORD]" << endl;
            cout << "   METHOD:" << endl;
            cout << "     scramble: will scramble the image" << endl;
            cout << "     unscramble: will unscramble the image" << endl;
            cout << "   FILENAME: path to an image file to process" << endl;
            cout << "   PASSWORD: the password used to process the image, will also use the SCRAMBLE_PASSWORD environment variable if set" << endl;
            exit(0);
        }

        switch (i) {
            case 1:
                if (arg != "scramble" && arg != "unscramble") {
                    cerr << "Invalid argument for METHOD" << endl;
                    exit(1);
                }
                scramble = arg == "scramble";
                break;

            case 2:
                filename = arg;
                break;

            case 3:
                password = arg;
                break;

            default:
                cerr << "Invalid argument: " + arg << endl;
                exit(1);
        }

    }

    if (filename.empty()) {
        cerr << "You must specify a filename" << endl;
        exit(1);
    }

    if (password.empty()) {
        char *pw_env = getenv("SCRAMBLE_PASSWORD");

        if (pw_env == NULL) {
            cerr << "You must specify a password, or set the SCRAMBLE_PASSWORD environment variable" << endl;
            exit(1);
        }

        password = pw_env;
    }

    Mat image = imread(filename, IMREAD_COLOR);
    if (image.empty()) {
        cerr << "Could not read image " << filename << endl;
        exit(1);
    }

    Key key = generateKey(password);
    vector<Row> pixels = toArray(image);

    pixels = doScramble<Row>(pixels, key, scramble);

    Mat imageOut = toImage(pixels, image.type());

    string outfile = getOutputFile(filename, scramble);
    imwrite(outfile, imageOut);

    cout << "Image written to " << outfile << endl;
}
