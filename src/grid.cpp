#include "grid.h"
#include <vector>
#include <glm/glm.hpp>
#include <iostream>
#include "simulator.h"

using namespace glm;

void Grid::init(int size, double timestep, double viscosity) {
	/*Grid contains 1-cell border of 0-values on all sides*/
	timeStep = timestep;
	viscosity = viscosity;
	grid_size = size;
	cell_size = 2.f / (grid_size + 2.f);

	velocities = std::vector<dvec3>((grid_size+2) * (grid_size+2), dvec3(0.f, 0.f, 0.f));
	//Randomly generate vector field
	for (int j = 0; j < grid_size; j++) {
		for (int i = 0; i < grid_size; i++) {
			float random_x = (float) (rand() % 20);
			float random_y = (float) (rand() % 20);
			random_x = (random_x - 10.f) / 10.f;
			random_y = (random_y - 10.f) / 10.f;
			velocities[index(i,j)] = dvec3(random_x, random_y, 0.f);
			if (length(velocities[index (i, j)]) != 0) {
				velocities[index (i, j)] = normalize(velocities[index (i, j)]);
			}
		}
	}

	old_velocities = velocities;
	pressures = std::vector<float>((grid_size+2) * (grid_size+2), 0.0);
	old_pressures = pressures;

	setVertices();
	setCentroids();
}

/******************************/
/* INIT, COORDINATES, VISUALS */
/******************************/

void Grid::setVertices() {
	/*Grid contains 1-cell border of 0-values on all sides*/
	vertices = std::vector<vec3> (0);
	vertices.reserve((grid_size+3) * (grid_size+3));
	double inc = cell_size;
	double current_height = -1.f;
	for (int j = 0; j <= grid_size+2; j++) {
		double current_width = -1.f;
		for (int i = 0; i <= grid_size+2; i++) {
			vertices.push_back(vec3(current_width, current_height, 0.0f));
			current_width += inc;
		}
		current_height += inc;
	}
}

void Grid::setCentroids() {
	/*Grid contains 1-cell border of 0-values on all sides*/
	centroid_vecs = std::vector<vec3> ((grid_size+2) * (grid_size+2), vec3(0.f));
	double inc = cell_size;
	for (int j = 0; j < grid_size+2; j++) {
		for (int i = 0; i < grid_size+2; i++) {
			centroid_vecs[j*(grid_size+2) + i] = vec3((i + 0.5) * inc - 1.0, (j + 0.5) * inc - 1.0, 0.f);
		}
	}
}

int Grid::index (int x, int y) {
	return (y + 1)*(grid_size + 2) + x + 1;
}

/**************/
/* Simulation */
/**************/

void Grid::calculateVelocity(float time) {
	//Test function
	for (unsigned int i = 0; i < velocities.size(); i++) {
		velocities[i].x = sin(time);
		velocities[i].y = cos(time);
	}
}

dvec3 lerp(double t, dvec3 a, dvec3 b) {
	return t * b + (1.f - t) * a;
	//return a + ((b - a) * t);
}

vec3 Grid::nearestBilerp(vec3 position) {
	// Find index of cell that holds position
	int box_x = (int) floor((position.x + 1.0)/cell_size) - 1;
	int box_y = (int) floor((position.y + 1.0)/cell_size) - 1;
	box_x = clamp(box_x, 0, grid_size - 1);
	box_y = clamp(box_y, 0, grid_size - 1);

	int horizontal_neighbor, vertical_neighbor;
	double t_x, t_y;

	/* HORIZONTAL NEIGHBOR TEST */
	float test_left = distance(centroid_vecs[index(box_x-1, box_y)], position);
	float test_right = distance(centroid_vecs[index(box_x+1, box_y)], position);
	if (test_left < test_right) {
		horizontal_neighbor = box_x - 1;
	} else {
		horizontal_neighbor = box_x + 1;
	}
	t_x = abs(position.x - centroid_vecs[index(box_x, box_y)].x)/cell_size;
	t_x = clamp(t_x, 0., 1.);

	/* VERTICAL NEIGHBOR TEST */
	float test_top = distance(centroid_vecs[index(box_x, box_y + 1)], position);
	float test_bottom = distance(centroid_vecs[index(box_x, box_y - 1)], position);
	if (test_top < test_bottom) {
		vertical_neighbor = box_y + 1;
	} else {
		vertical_neighbor = box_y - 1;
	}
	t_y = abs(position.y - centroid_vecs[index(box_x, box_y)].y)/cell_size;
	t_y = clamp(t_y, 0., 1.);

	// TODO: make this more general to include both velocities and pressures, etc.
	dvec3 first_ho = lerp(t_x, velocities[index(horizontal_neighbor, box_y)],
								velocities[index(box_x, box_y)]);
	dvec3 second_ho = lerp(t_x, velocities[index(horizontal_neighbor, vertical_neighbor)],
								velocities[index(box_x, vertical_neighbor)]);
	dvec3 vertical = lerp(t_y, second_ho, first_ho);
	return vertical;
}

void Grid::calculateAdvection() {
	old_velocities = velocities;
	for (int j = 0; j < grid_size; ++j) {
		for (int i = 0; i < grid_size; ++i) {
			int n = index(i, j);
			dvec3 currPos = centroid_vecs[n];
			dvec3 currVelocity = velocities[n] * timeStep;
			dvec3 prevPos = currPos - currVelocity;
			velocities[n] = nearestBilerp(prevPos);
		}
	}
}

void Grid::calculateDiffusion(int iterations) {
	for (int j = 0; j < grid_size; j++) {
		for (int i = 0; i < grid_size; i++) {
			for (int n = 0; n < iterations; n++) {
				jacobiStepDiffuse(i, j);
			}
		}
	}
}

void Grid::jacobiStepDiffuse(int i, int j) {
	//TODO
	int n = index(i, j);
	dvec3 alpha = (velocities[n] - old_velocities[n]) * (velocities[n] - old_velocities[n]) / timeStep / viscosity;
	dvec3 beta = dvec3(1./(4. + alpha.x), 1./(4. + alpha.y), 0);
	dvec3 L = velocities[n - 1];
	dvec3 R = velocities[n + 1];
	dvec3 T = velocities[index(i, j+1)];
	dvec3 B = velocities[index(i, j-1)];
	dvec3 self = velocities[n];
	old_velocities[n] = velocities[n];
	velocities[n] = (L + R + B + T + alpha * self) * beta;
}

void Grid::calculatePressure(int iterations) {
	//TODO
	for (unsigned int j = 0; j < pressures.size(); j++) {
		for (unsigned int i = 0; i < pressures.size(); i++) {
			int n = index(i, j);
			float alpha = -pow(pressures[n] - old_pressures[n], 2);
			float beta = 1./4.;
			float L = pressures[n - 1];
			float R = pressures[n + 1];
			float T = pressures[index(i, j+1)];
			float B = pressures[index(i, j-1)];
			float self = pressures[n];
			old_pressures[n] = pressures[n];
			pressures[n] = (L + R + B + T + alpha * self) * beta;
		}
	}
}