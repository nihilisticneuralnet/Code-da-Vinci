#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <omp.h>

using namespace cv;
using namespace std;

// Random Color Generator
Scalar getRandomColor() {
    return Scalar(rand() % 256, rand() % 256, rand() % 256);
}

class Subject {
public:
    Mat image;
    double fitness;
    Size size;

    Subject(Size size) : size(size) {
        image = Mat(size, CV_8UC3, getRandomColor());
        fitness = -1;
    }

    Subject(Mat img) : image(img), fitness(-1) {
        size = img.size();
    }

    void drawPolygon(int nbPolygon = -1, int subdivise = 8) {
        if (nbPolygon == -1) {
            nbPolygon = rand() % 4 + 3;
        }
        Point region(size.width / subdivise, size.height / subdivise);
        for (int i = 0; i < nbPolygon; i++) {
            int nbPoints = rand() % 4 + 3;
            Point pos(rand() % size.width, rand() % size.height);
            vector<Point> points;
            for (int j = 0; j < nbPoints; j++) {
                points.push_back(Point(pos.x + rand() % region.x - region.x / 2,
                                       pos.y + rand() % region.y - region.y / 2));
            }
            fillConvexPoly(image, points, getRandomColor());
        }
    }

    double getFitness(const Mat& targetImage) {
        if (fitness == -1) {
            fitness = mean(cv::norm(image, targetImage, NORM_L2))[0];
        }
        return fitness;
    }
};

class Generation {
public:
    Mat targetImage;
    Size size;
    vector<Subject> population;
    int nbSubject;

    Generation(const string& imagePath) {
        targetImage = imread(imagePath);
        size = targetImage.size();
        nbSubject = 0;
    }

    void createPopulation(int nbSubject, const string& loadImagePath = "") {
        this->nbSubject = nbSubject;
        if (!loadImagePath.empty()) {
            for (int i = 0; i < nbSubject - 1; i++) {
                Mat img = imread(loadImagePath);
                population.push_back(Subject(img));
                population.back().drawPolygon(1);
            }
            Mat img = imread(loadImagePath);
            population.push_back(Subject(img));
        } else {
            for (int i = 0; i < nbSubject; i++) {
                population.push_back(Subject(size));
                population.back().drawPolygon();
            }
        }
        cout << "Population created with " << nbSubject << " subjects." << endl;
    }

    vector<Subject> fitness() {
        vector<Subject> sortedPopulation = population;
        sort(sortedPopulation.begin(), sortedPopulation.end(), [this](Subject& a, Subject& b) {
            return a.getFitness(targetImage) < b.getFitness(targetImage);
        });
        return sortedPopulation;
    }

//     void crossover(const Subject& parent) {
//     vector<Subject> newPopulation = {parent};
//     #pragma omp parallel for
//     for (int i = 1; i < nbSubject; i++) {
//         int crossover = rand() % size.width;

//         // Create children images
//         Mat child1 = Mat::zeros(size, CV_8UC3);
//         Mat child2 = Mat::zeros(size, CV_8UC3);

//         // Perform crossover
//         parent.image(Rect(0, 0, crossover, size.height)).copyTo(child1(Rect(0, 0, crossover, size.height)));
//         population[i].image(Rect(crossover, 0, size.width - crossover, size.height)).copyTo(child1(Rect(crossover, 0, size.width - crossover, size.height)));

//         population[i].image(Rect(0, 0, crossover, size.height)).copyTo(child2(Rect(0, 0, crossover, size.height)));
//         parent.image(Rect(crossover, 0, size.width - crossover, size.height)).copyTo(child2(Rect(crossover, 0, size.width - crossover, size.height)));

//         #pragma omp critical
//         {
//             newPopulation.push_back(Subject(child1));
//             newPopulation.push_back(Subject(child2));
//         }
//     }
//     population = newPopulation;
// }
    void crossover(const Subject& parent) {
    vector<Subject> newPopulation = {parent};

    #pragma omp parallel for
    for (int i = 1; i < nbSubject; i++) {
        int crossover = rand() % size.width;

        // Ensure deep clones for both children
        Mat child1 = Mat::zeros(size, CV_8UC3);
        Mat child2 = Mat::zeros(size, CV_8UC3);

        Mat subImg1Parent = parent.image(Rect(0, 0, crossover, size.height)).clone();
        Mat subImg2Other = population[i].image(Rect(crossover, 0, size.width - crossover, size.height)).clone();
        subImg1Parent.copyTo(child1(Rect(0, 0, crossover, size.height)));
        subImg2Other.copyTo(child1(Rect(crossover, 0, size.width - crossover, size.height)));

        Mat subImg1Other = population[i].image(Rect(0, 0, crossover, size.height)).clone();
        Mat subImg2Parent = parent.image(Rect(crossover, 0, size.width - crossover, size.height)).clone();
        subImg1Other.copyTo(child2(Rect(0, 0, crossover, size.height)));
        subImg2Parent.copyTo(child2(Rect(crossover, 0, size.width - crossover, size.height)));

        #pragma omp critical
        {
            newPopulation.push_back(Subject(child1));
            newPopulation.push_back(Subject(child2));
        }
    }
    population = newPopulation;
}


    void mutation() {
        bool isParent = true;
        #pragma omp parallel for
        for (int i = 0; i < population.size(); i++) {
            if (isParent) {
                isParent = false;
            } else {
                population[i].drawPolygon(1);
            }
        }
    }

    void main(int nbGeneration) {
        cout << "Starting Generation" << endl;
        for (int i = 0; i < nbGeneration; i++) {
            cout << "Generation: " << i + 1 << "/" << nbGeneration << "\t";
            vector<Subject> sortedPopulation = fitness();

            cout << "Fitness: " << sortedPopulation[0].getFitness(targetImage) << endl;
            if (i + 1 == nbGeneration) {
                imwrite("res.png", sortedPopulation[0].image);
                break;
            } else if (i % 5 == 0) {
                imwrite("generation" + to_string(i) + ".png", sortedPopulation[0].image);
            }
            population = vector<Subject>(sortedPopulation.begin(), sortedPopulation.begin() + nbSubject);
            crossover(sortedPopulation[0]);
            mutation();
        }
    }
};

int main() {
    srand((unsigned)time(NULL));
    Generation gen("mona.png");
    gen.createPopulation(100);
    gen.main(100);
    return 0;
}
