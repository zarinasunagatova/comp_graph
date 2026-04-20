/* парсер OBJ-файлов 
с функцией автоматического вычисления параметров 
для визуализации.*/
#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <Windows.h>

using namespace std;

struct Vertex { float x, y, z; };
struct Face { int v[3]; };

class OBJParser {
public:
    vector<Vertex> vertices;
    vector<Face> faces;

    float centerX, centerY, centerZ;
    float minX, maxX, minY, maxY, minZ, maxZ;
    float sizeX, sizeY, sizeZ;
    float recommendedScale;
    float recommendedAX;
    float recommendedZOffset;

    float shiftAfterScaleX, shiftAfterScaleY, shiftAfterScaleZ;

    void loadWithoutCentering(const string& filename) {
        vertices.clear();
        faces.clear();

        string line;
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "Не удалось открыть файл " << filename << endl;
            return;
        }

        while (getline(file, line)) {
            if (line.empty()) continue;

            if (line[0] == 'v' && line[1] == ' ') {
                Vertex v;
                stringstream ss(line);
                char type;
                ss >> type >> v.x >> v.y >> v.z;
                vertices.push_back(v);
            }
            else if (line[0] == 'f' && line[1] == ' ') {
                Face f;
                stringstream ss(line);
                char type;
                ss >> type;

                vector<int> indices;
                string token;
                while (ss >> token) {
                    size_t slashPos = token.find('/');
                    if (slashPos != string::npos) {
                        token = token.substr(0, slashPos);
                    }
                    int idx = stoi(token);
                    indices.push_back(idx - 1);
                }

                if (indices.size() == 3) {
                    f.v[0] = indices[0];
                    f.v[1] = indices[1];
                    f.v[2] = indices[2];
                    faces.push_back(f);
                }
                else if (indices.size() == 4) {
                    Face f1 = { indices[0], indices[1], indices[2] };
                    Face f2 = { indices[0], indices[2], indices[3] };
                    faces.push_back(f1);
                    faces.push_back(f2);
                }
            }
        }

        if (vertices.empty()) {
            cerr << "Ошибка загрузки вершин!" << endl;
            return;
        }

        cout << "\nПЕРВЫЕ 5 ВЕРШИН" << endl;
        for (int i = 0; i < min(5, (int)vertices.size()); i++) {
            cout << "v" << i << ": " << vertices[i].x << ", "
                << vertices[i].y << ", " << vertices[i].z << endl;
        }

        minX = vertices[0].x; maxX = vertices[0].x;
        minY = vertices[0].y; maxY = vertices[0].y;
        minZ = vertices[0].z; maxZ = vertices[0].z;

        for (const auto& v : vertices) {
            minX = min(minX, v.x); maxX = max(maxX, v.x);
            minY = min(minY, v.y); maxY = max(maxY, v.y);
            minZ = min(minZ, v.z); maxZ = max(maxZ, v.z);
        }

        centerX = (minX + maxX) * 0.5f;
        centerY = (minY + maxY) * 0.5f;
        centerZ = (minZ + maxZ) * 0.5f;

        sizeX = maxX - minX;
        sizeY = maxY - minY;
        sizeZ = maxZ - minZ;

        float maxSize = sizeX;
        if (sizeY > maxSize) maxSize = sizeY;
        if (sizeZ > maxSize) maxSize = sizeZ;

        if (maxSize > 0) {
            recommendedScale = (1000.0f * 0.8f) / maxSize;
        }
        else {
            recommendedScale = 1.0f;
        }

        shiftAfterScaleX = -centerX * recommendedScale;
        shiftAfterScaleY = -centerY * recommendedScale;
        shiftAfterScaleZ = -centerZ * recommendedScale;

        
        float distance = sizeZ * 3.0f;
        if (distance <= 0) distance = 100.0f;

        float maxXY = (sizeX > sizeY) ? sizeX : sizeY;
        float halfSize = maxXY / 2.0f;

        if (halfSize > 0) {
            recommendedAX = (500.0f * distance) / halfSize;
        }
        else {
            recommendedAX = 400.0f;
        }
        recommendedAX = min(recommendedAX, 800.0f);
        recommendedZOffset = distance;
    }

    void printCenteringParams() {
        cout << "\nПАРАМЕТРЫ ДЛЯ ЦЕНТРИРОВАНИЯ\n";
        cout << "Порядок операций для ВСЕХ заданий: МАСШТАБ → ПОВОРОТ → СДВИГ\n\n";

        cout << "ОРИГИНАЛЬНЫЙ BOUNDING BOX\n";
        cout << "X: [" << minX << ", " << maxX << "]  (ширина: " << sizeX << ")\n";
        cout << "Y: [" << minY << ", " << maxY << "]  (высота: " << sizeY << ")\n";
        cout << "Z: [" << minZ << ", " << maxZ << "]  (глубина: " << sizeZ << ")\n\n";

        cout << "ЦЕНТР МОДЕЛИ\n";
        cout << "centerX = " << centerX << "f;\n";
        cout << "centerY = " << centerY << "f;\n";
        cout << "centerZ = " << centerZ << "f;\n\n";

        cout << "ОБЩИЕ ПАРАМЕТРЫ (для всех заданий)\n";
        cout << "float scale = " << recommendedScale << "f;    // масштаб\n";
        cout << "float tX = " << shiftAfterScaleX << "f;       // сдвиг по X (ПОСЛЕ масштаба)\n";
        cout << "float tY = " << shiftAfterScaleY << "f;       // сдвиг по Y (ПОСЛЕ масштаба)\n";
        cout << "float tZ = " << shiftAfterScaleZ << "f;       // сдвиг по Z (ПОСЛЕ масштаба)\n\n";

        cout << "ПАРАМЕТРЫ ДЛЯ task15 (ортографическая проекция)\n";
        cout << "// После масштаба и поворота просто добавляем сдвиг:\n";
        cout << "v0.x += tX; v0.y += tY; v0.z += tZ;\n\n";

        cout << "ПАРАМЕТРЫ ДЛЯ task16 (перспективная проекция)\n";
        cout << "float aX = " << recommendedAX << "f;\n";
        cout << "float aY = " << recommendedAX << "f;\n";
        cout << "float z_offset = " << recommendedZOffset << "f;\n\n";

        cout << "ГОТОВЫЕ СТРОКИ ДЛЯ КОПИРОВАНИЯ\n";
        cout << "// Для task15 и task16 (общие):\n";
        cout << "float scale = " << recommendedScale << "f;\n";
        cout << "float tX = " << shiftAfterScaleX << "f;\n";
        cout << "float tY = " << shiftAfterScaleY << "f;\n";
        cout << "float tZ = " << shiftAfterScaleZ << "f;\n\n";

        cout << "// Для task16 (дополнительно):\n";
        cout << "float aX = " << recommendedAX << "f;\n";
        cout << "float aY = " << recommendedAX << "f;\n";
        cout << "float z_offset = " << recommendedZOffset << "f;\n";

    }
};


int main() {
    setlocale(LC_ALL, "Rus");
    OBJParser parser;
    parser.loadWithoutCentering("model_1 (1).obj");

    if (parser.vertices.empty()) {
        cerr << "Не удалось загрузить модель!" << endl;
        return 1;
    }

    parser.printCenteringParams();

    return 0;
}