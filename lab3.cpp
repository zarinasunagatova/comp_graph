#include "lodepng.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <random>
#include <corecrt_math_defines.h>
#include <Windows.h>

struct Vertex {
    float x, y, z;
    Vertex() : x(0), y(0), z(0) {}
    Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

Vertex rotate_y(const Vertex& v, float angle_rad) {
    float cos_a = cos(angle_rad);
    float sin_a = sin(angle_rad);
    return Vertex(
        v.x * cos_a + v.z * sin_a,
        v.y,
        -v.x * sin_a + v.z * cos_a
    );
}

Vertex rotate_full(const Vertex& v, float alpha, float beta, float gamma) {
    float ca = cos(alpha), sa = sin(alpha);
    float cb = cos(beta), sb = sin(beta);
    float cg = cos(gamma), sg = sin(gamma);

    float x1 = v.x * cg - v.y * sg;
    float y1 = v.x * sg + v.y * cg;
    float z1 = v.z;

    float x2 = x1 * cb + z1 * sb;
    float y2 = y1;
    float z2 = -x1 * sb + z1 * cb;

    float x3 = x2;
    float y3 = y2 * ca - z2 * sa;
    float z3 = y2 * sa + z2 * ca;

    return Vertex(x3, y3, z3);
}


bool load_obj(const std::string& filename,
	std::vector<Vertex>& vertices,
	std::vector<std::vector<int>>& polygons) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Не удалось открыть файл: " << filename << std::endl;
		return false;
	}
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			vertices.push_back(Vertex(x, y, z));
		}
		else if (type == "f") {
			std::vector<int> polygon;
			std::string part;
			while (iss >> part) {
				size_t slash_pos = part.find('/');
				int v_idx = std::stoi(part.substr(0, slash_pos)) - 1;
				polygon.push_back(v_idx);
			}
			polygons.push_back(polygon);
		}
	}
	file.close();
	std::cout << "Загружено вершин: " << vertices.size()
		<< ", полигонов: " << polygons.size() << std::endl;
	return true;
}

struct BarycentricCoords {
	float lambda0, lambda1, lambda2;
};

BarycentricCoords compute_barycentric(int x, int y,
	float x0, float y0,
	float x1, float y1,
	float x2, float y2) {

	BarycentricCoords coords;
	float denominator = (x0 - x2) * (y1 - y2) - (x1 - x2) * (y0 - y2);
	if (std::abs(denominator) < 1e-6f) {
		coords.lambda0 = -1.0f;
		coords.lambda1 = -1.0f;
		coords.lambda2 = -1.0f;
		return coords;
	}
	coords.lambda0 = ((x - x2) * (y1 - y2) - (x1 - x2) * (y - y2)) / denominator;
	coords.lambda1 = ((x0 - x2) * (y - y2) - (x - x2) * (y0 - y2)) / denominator;
	coords.lambda2 = 1.0f - coords.lambda0 - coords.lambda1;

	return coords;
}

void draw_triangle(std::vector<unsigned char>& img, int w, int h,
	float x0, float y0, float x1, float y1, float x2, float y2,
	unsigned char r, unsigned char g, unsigned char b,
	std::vector<float>* zbuffer = nullptr,
	float z0 = 0, float z1 = 0, float z2 = 0) {

	int xmin = (int)std::floor((std::min)({ x0, x1, x2 }));
	int xmax = (int)std::ceil((std::max)({ x0, x1, x2 }));
	int ymin = (int)std::floor((std::min)({ y0, y1, y2 }));
	int ymax = (int)std::ceil((std::max)({ y0, y1, y2 }));
	xmin = (std::max)(0, xmin);
	xmax = (std::min)(w - 1, xmax);
	ymin = (std::max)(0, ymin);
	ymax = (std::min)(h - 1, ymax);

	for (int y = ymin; y <= ymax; y++) {
		for (int x = xmin; x <= xmax; x++) {
			BarycentricCoords coords = compute_barycentric(x, y, x0, y0, x1, y1, x2, y2);

			if (coords.lambda0 >= 0 && coords.lambda1 >= 0 && coords.lambda2 >= 0) {
				bool draw_pixel = true;

				if (zbuffer != nullptr) {
					float z = coords.lambda0 * z0 + coords.lambda1 * z1 + coords.lambda2 * z2;
					size_t idx = y * w + x;

					if (z < (*zbuffer)[idx]) {
						(*zbuffer)[idx] = z;
					}
					else {
						draw_pixel = false;
					}
				}

				if (draw_pixel) {
					size_t idx = (y * w + x) * 3;
					img[idx + 0] = r;
					img[idx + 1] = g;
					img[idx + 2] = b;
				}
			}
		}
	}
}

void task15(
	const std::vector<Vertex>& vertices,
	const std::vector<std::vector<int>>& polygons,
	int img_size = 1000) {

	if (vertices.empty() || polygons.empty()) {
		std::cerr << "Нет вершин или полигонов!" << std::endl;
		return;
	}

	float offset_x = img_size / 2.0f;
	float offset_y = img_size / 2.0f;
	float scale = 500.0f;

	float alpha = 0.0f;
	float beta = 90.0f * M_PI / 180.0f;
	float gamma = 0.0f;

	float min_x = vertices[0].x, max_x = vertices[0].x;
	float min_y = vertices[0].y, max_y = vertices[0].y;

	for (const auto& v : vertices) {
		min_x = (std::min)(min_x, v.x);
		max_x = (std::max)(max_x, v.x);
		min_y = (std::min)(min_y, v.y);
		max_y = (std::max)(max_y, v.y);
	}

	float center_x = (min_x + max_x) / 2.0f;
	float center_y = (min_y + max_y) / 2.0f;

	float tX = -center_x;
	float tY = -center_y;
	float tZ = 0.0f;

	std::vector<unsigned char> img(img_size * img_size * 3, 0);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> color_dist(0, 255);

	int triangle_count = 0;
	for (const auto& poly : polygons) {
		if (poly.size() != 3) continue;

		Vertex v0 = vertices[poly[0]];
		Vertex v1 = vertices[poly[1]];
		Vertex v2 = vertices[poly[2]];

		v0.x *= scale; v0.y *= scale; v0.z *= scale;
		v1.x *= scale; v1.y *= scale; v1.z *= scale;
		v2.x *= scale; v2.y *= scale; v2.z *= scale;

		v0 = rotate_full(v0, alpha, beta, gamma);
		v1 = rotate_full(v1, alpha, beta, gamma);
		v2 = rotate_full(v2, alpha, beta, gamma);

		v0.x += tX; v0.y += tY; v0.z += tZ;
		v1.x += tX; v1.y += tY; v1.z += tZ;
		v2.x += tX; v2.y += tY; v2.z += tZ;

		float x0 = v0.x + offset_x;
		float y0 = img_size - 1 - (v0.y + offset_y);
		float x1 = v1.x + offset_x;
		float y1 = img_size - 1 - (v1.y + offset_y);
		float x2 = v2.x + offset_x;
		float y2 = img_size - 1 - (v2.y + offset_y);

		unsigned char r = color_dist(gen);
		unsigned char g = color_dist(gen);
		unsigned char b = color_dist(gen);

		draw_triangle(img, img_size, img_size, x0, y0, x1, y1, x2, y2, r, g, b);
		triangle_count++;
		if (triangle_count % 1000 == 0) {
			std::cout << "Отрисовано " << triangle_count << " треугольников..." << std::endl;
		}
	}

	std::cout << "Всего отрисовано " << triangle_count << " треугольников." << std::endl;
	lodepng::encode("task15.png", img, img_size, img_size, LCT_RGB);
	std::cout << "Сохранено: task15.png" << std::endl;
}

void task16(
	const std::vector<Vertex>& vertices,
	const std::vector<std::vector<int>>& polygons,
	int img_size = 1200) {

	float u0 = img_size / 2.0f;
	float w0 = img_size / 2.0f;
	float aX = 1000.0f;
	float aY = 1000.0f;
	float alpha = 0.0f;
	float beta = 180.0f * M_PI / 180.0f;
	float gamma = 0.0f;
	float z_offset = 4.0f;

	float min_x = vertices[0].x, max_x = vertices[0].x;
	float min_y = vertices[0].y, max_y = vertices[0].y;
	float min_z = vertices[0].z, max_z = vertices[0].z;

	for (const auto& v : vertices) {
		min_x = (std::min)(min_x, v.x);
		max_x = (std::max)(max_x, v.x);
		min_y = (std::min)(min_y, v.y);
		max_y = (std::max)(max_y, v.y);
		min_z = (std::min)(min_z, v.z);
		max_z = (std::max)(max_z, v.z);
	}

	float center_x = (min_x + max_x) / 2.0f;
	float center_y = (min_y + max_y) / 2.0f;
	float center_z = (min_z + max_z) / 2.0f;

	float tX = -center_x;
	float tY = -center_y;
	float tZ = -center_z;

	std::vector<unsigned char> img(img_size * img_size * 3, 0);
	std::vector<float> zbuffer(img_size * img_size, (std::numeric_limits<float>::max)());

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> color_dist(0, 255);

	int triangle_count = 0;
	for (const auto& poly : polygons) {
		if (poly.size() != 3) continue;

		Vertex v0 = vertices[poly[0]];
		Vertex v1 = vertices[poly[1]];
		Vertex v2 = vertices[poly[2]];

		v0 = rotate_full(v0, alpha, beta, gamma);
		v1 = rotate_full(v1, alpha, beta, gamma);
		v2 = rotate_full(v2, alpha, beta, gamma);

		v0.x += tX; v0.y += tY; v0.z += tZ;
		v1.x += tX; v1.y += tY; v1.z += tZ;
		v2.x += tX; v2.y += tY; v2.z += tZ;

		float z0 = v0.z + z_offset;
		float z1 = v1.z + z_offset;
		float z2 = v2.z + z_offset;

		if (z0 <= 0 || z1 <= 0 || z2 <= 0) continue;

		float x0 = aX * v0.x / z0 + u0;
		float y0 = aY * v0.y / z0 + w0;
		float x1 = aX * v1.x / z1 + u0;
		float y1 = aY * v1.y / z1 + w0;
		float x2 = aX * v2.x / z2 + u0;
		float y2 = aY * v2.y / z2 + w0;

		y0 = img_size - 1 - y0;
		y1 = img_size - 1 - y1;
		y2 = img_size - 1 - y2;

	
		unsigned char r = color_dist(gen);
		unsigned char g = color_dist(gen);
		unsigned char b = color_dist(gen);

		draw_triangle(img, img_size, img_size,
			x0, y0, x1, y1, x2, y2,
			r, g, b, &zbuffer, z0, z1, z2);
		triangle_count++;
		
	}

	std::cout << "Отрисовано треугольников: " << triangle_count << std::endl;
	lodepng::encode("task16.png", img, img_size, img_size, LCT_RGB);
	std::cout << "Сохранено: task16.png" << std::endl;
}

int main() {
	setlocale(LC_ALL, "Rus");
	std::string obj_filename = "model.obj";

	std::vector<Vertex> vertices;
	std::vector<std::vector<int>> polygons;

	if (!load_obj(obj_filename, vertices, polygons)) {
		std::cerr << "Ошибка загрузки модели!" << std::endl;
		return 1;
	}

	task15(vertices, polygons, 1200);
	task16(vertices, polygons, 1200);

	std::cout << "Готово!" << std::endl;
	return 0;
}