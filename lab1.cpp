#define _USE_MATH_DEFINES
#include "lodepng.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <limits>
#include <Windows.h>

void task1(int H = 600, int W = 800) {
	std::cout << "Задание 1. Работа с изображениями.\n";

	std::vector<unsigned char> black(H * W, 0);
	lodepng::encode("1_black.png", black, W, H, LCT_GREY);
	std::cout << "Сохранено: 1_black.png (черное)\n";

	

	std::vector<unsigned char> white(H*W, 255);
	lodepng::encode("1_white.png", white, W, H, LCT_GREY);
	std::cout << "Сохранено: 1_white.png (белое)\n";



	std::vector<unsigned char> red(H*W*3);
	for (size_t i = 0; i < H * W; i++) {
		red[i * 3 + 0] = 255;
		red[i * 3 + 1] = 0;
		red[i * 3 + 2] = 0;
	}
	lodepng::encode("1_red.png", red, W, H, LCT_RGB);
	std::cout << "Сохранено: 1_red.png (красное)\n";



	std::vector<unsigned char> gradient(H*W*3);
	for (int y = 0; y < H; y++) {
		for (int x = 0; x < W; x++) {
			size_t idx = (y * W + x) * 3;
			gradient[idx + 0] = (x + y) % 256;
			gradient[idx + 1] = (2 * x) % 256;
			gradient[idx + 2] = (2 * y) % 256;
		}
	}
	lodepng::encode("1_gradient.png", gradient, W, H, LCT_RGB);
	std::cout << "Сохранено: 1_gradient.png (градиент)\n";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void draw_line_v1(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	int count = 100;
	float step =  1.0f / count;
	for (float t = 0; t < 1.0f; t += step) {
		int x = (int)std::round((1.0f - t) * x0 + t * x1);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);
		if (x >= 0 && x < w && y >= 0 && y < h) {
			img[y * w + x] = color;
		}
	}
}

void dotted_line(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	float count = std::sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
	float step = 1.0f / count;
	for (float t = 0; t < 1.0f; t += step) {
		int x = (int)std::round((1.0f - t) * x0 + t * x1);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);
		if (x >= 0 && x < w && y >= 0 && y < h) {
			img[y * w + x] = color;
		}
	}
}

void x_loop_line(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	int x0_int = (int)x0;
	int x1_int = (int)x1;

	for (int x = x0_int; x <= x1_int; x++) {
		float t = (float)(x - x0_int) / (x1_int - x0_int);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);
		if (x >= 0 && x < w && y >= 0 && y < h) {
			img[y * w + x] = color;
		}
	}
}

void x_loop_line_1(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int x0_int = (int)x0;
	int x1_int = (int)x1;

	for (int x = x0_int; x <= x1_int; x++) {
		float t = (float)(x - x0_int) / (x1_int - x0_int);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);
		if (x >= 0 && x < w && y >= 0 && y < h) {
			img[y * w + x] = color;
		}
	}
}

void x_loop_line_2(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	bool xchange = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		xchange = true;
	}

	int x0_int = (int)x0;
	int x1_int = (int)x1;

	for (int x = x0_int; x <= x1_int; x++) {
		float t = (float)(x - x0_int) / (x1_int - x0_int);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);

		if (xchange) {
			if (x >= 0 && x < h && y >= 0 && y < w)
				img[x * w + y] = color;
		}
		else {
			if (y >= 0 && y < h && x >= 0 && x < w)
				img[y * w + x] = color;
		}
	}
}

void x_loop_line_12(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	bool xchange = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		xchange = true;
	}

	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int x0_int = (int)x0;
	int x1_int = (int)x1;

	for (int x = x0_int; x <= x1_int; x++) {
		float t = (float)(x - x0_int) / (x1_int - x0_int);
		int y = (int)std::round((1.0f - t) * y0 + t * y1);

		if (xchange) {
			if (x >= 0 && x < h && y >= 0 && y < w)
				img[x * w + y] = color;
		}
		else {
			if (y >= 0 && y < h && x >= 0 && x < w)
				img[y * w + x] = color;
		}
	}
}

void x_loop_line_no_y(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	bool xchange = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		xchange = true;
	}

	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int x0_int = (int)x0;
	int x1_int = (int)x1;
	int y = (int)y0;
	float dy = std::abs(y1 - y0) / (float)(x1_int - x0_int);
	float derror = 0.0f;
	int y_update = (y1 > y0) ? 1 : -1;

	for (int x = x0_int; x <= x1_int; x++) {
		if (xchange) {
			if (x >= 0 && x < h && y >= 0 && y < w)
				img[x * w + y] = color;
		}
		else {
			if (y >= 0 && y < h && x >= 0 && x < w)
				img[y * w + x] = color;
		}

		derror += dy;
		if (derror > 0.5f) {
			derror -= 1.0f;
			y += y_update;
		}
	}
}

void x_loop_line_no_y_v2(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	bool xchange = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		xchange = true;
	}

	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int x0_int = (int)x0;
	int x1_int = (int)x1;
	int y = (int)y0;
	int dy = std::abs(y1 - y0);  
	int derror = 0;
	int y_update = (y1 > y0) ? 1 : -1;

	for (int x = x0_int; x < x1_int; x++) {
		if (xchange) {
			if (x >= 0 && x < h && y >= 0 && y < w)
				img[x * w + y] = color;
		}
		else {
			if (y >= 0 && y < h && x >= 0 && x < w)
				img[y * w + x] = color;
		}

		derror += dy;
		if (derror >= (x1_int - x0_int)) {
			derror -= (x1_int - x0_int);
			y += y_update;
		}
	}
}

void bresenham_line(std::vector<unsigned char>& img, int w, int h,
	int x0, int y0, int x1, int y1, unsigned char color) {
	bool xchange = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		xchange = true;
	}

	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = std::abs(y1 - y0);
	int error = dx / 2;
	int y = y0;
	int y_step = (y0 < y1) ? 1 : -1;

	for (int x = x0; x <= x1; x++) {
		if (xchange) {
			if (x >= 0 && x < h && y >= 0 && y < w)
				img[x * w + y] = color;
		}
		else {
			if (y >= 0 && y < h && x >= 0 && x < w)
				img[y * w + x] = color;
		}

		error -= dy;
		if (error < 0) {
			y += y_step;
			error += dx;
		}
	}
}

void draw_star(int size, const std::string& filename,
	void (*draw_func)(std::vector<unsigned char>&, int, int, int, int, int, int, unsigned char),
	int count = 100) { 

	std::vector<unsigned char> img(size * size, 0);
	int center = size / 2;
	int radius = 95;
	unsigned char white = 255;

	const int num_lines = 13;
	for (int i = 0; i < num_lines; i++) {
		double angle = 2.0 * M_PI * i / num_lines;
		int x_end = center + radius * cos(angle);
		int y_end = center + radius * sin(angle);

		draw_func(img, size, size, center, center, x_end, y_end, white);
	}
	lodepng::encode(filename, img, size, size, LCT_GREY);
	std::cout << "Сохранено: " << filename << std::endl;
}

void task2() {
	std::cout << "Задание 2. Отрисовка прямых линий.\n";
	draw_star(200, "star_v1.png", draw_line_v1);
	draw_star(200, "star_v2.png", dotted_line);
	draw_star(200, "star_v3.png", x_loop_line);
	draw_star(200, "star_v4.png", x_loop_line_1);
	draw_star(200, "star_v5.png", x_loop_line_2);
	draw_star(200, "star_v6.png", x_loop_line_12);
	draw_star(200, "star_v7.png", x_loop_line_no_y);
	draw_star(200, "star_v8.png", x_loop_line_no_y_v2);
	draw_star(200, "star_v9.png", bresenham_line);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Vertex {
	float x, y, z;
	Vertex() : x(0), y(0), z(0) {}
	Vertex(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

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

void task4(const std::vector<Vertex>& vertices,
	int img_size = 1000, float scale = 5000.0f,
	float offset_x = 500.0f, float offset_y = 500.0f) {
	std::cout << "Задание 4. Отрисовка вершин.\n";
	std::vector<unsigned char> img(img_size * img_size, 0);
	for (const auto& v : vertices) {
		int x = (int)(scale * v.x + offset_x);
		int y = img_size - 1 - (int)(scale * v.y + offset_y); 
		if (x >= 0 && x < img_size && y >= 0 && y < img_size) {
			img[y * img_size + x] = 255;
		}
	}
	lodepng::encode("4_vertices.png", img, img_size, img_size, LCT_GREY);
	std::cout << "Сохранено: 4_vertices.png\n";
}

void task6(const std::vector<Vertex>& vertices,
	const std::vector<std::vector<int>>&polygons,
	int img_size = 1000, float scale = 5000.0f, float offset_x = 500.0f, float offset_y = 500.0f) {
	std::cout << "Задание 6. Отрисовка ребер\n";
	std::vector<unsigned char> img(img_size*img_size, 0);
	auto bresenham_line = [&](int x0, int y0, int x1, int y1) {
		if ((x0 < 0 && x1 < 0) || (x0 >= img_size && x1 >= img_size) ||
			(y0 < 0 && y1 < 0) || (y0 >= img_size && y1 >= img_size)) {
			return;
		}

		int dx = std::abs(x1 - x0);
		int dy = std::abs(y1 - y0);
		int sx = (x0 < x1) ? 1 : -1;
		int sy = (y0 < y1) ? 1 : -1;
		int err = dx - dy;

		int x = x0, y = y0;

		while (true) {
			if (x >= 0 && x < img_size && y >= 0 && y < img_size) {
				img[y * img_size + x] = 255;
			}

			if (x == x1 && y == y1) break;

			int e2 = 2 * err;
			if (e2 > -dy) {
				err -= dy;
				x += sx;
			}
			if (e2 < dx) {
				err += dx;
				y += sy;
			}
		}
	};

	for (const auto& poly : polygons) {
		int n = (int)poly.size();
		for (int i = 0; i < n; i++) {
			int v1 = poly[i];
			int v2 = poly[(i + 1) % n];

			int x1 = (int)(scale * vertices[v1].x + offset_x);
			int y1 = img_size - 1 - (int)(scale * vertices[v1].y + offset_y);  
			int x2 = (int)(scale * vertices[v2].x + offset_x);
			int y2 = img_size - 1 - (int)(scale * vertices[v2].y + offset_y);
			bresenham_line(x1, y1, x2, y2);
		}
	}
	lodepng::encode("6_edges.png", img, img_size, img_size, LCT_GREY);
	std::cout << "Сохранено: 6_edges.png\n";
}


int main(int argc, char* argv[]) {
	setlocale(LC_ALL, "Rus");
	task1(600, 800);
	task2();
	std::string obj_filename = "model_1.obj";

	std::vector<Vertex> vertices;
	std::vector<std::vector<int>> polygons;
	if (load_obj(obj_filename, vertices, polygons)) {
		std::cout << "Задание 3, 6\n";
		std::cout << "Загружено " << vertices.size() << " вершин, "
		<< polygons.size() << " полигонов\n";
		task4(vertices);
		task6(vertices, polygons);
	}
	else {
		std::cerr << "Ошибка: не удалось загрузить модель. Убедитесь, что файл "
			<< obj_filename << " существует.\n";
		return 1;
	}
	return 0;
}
