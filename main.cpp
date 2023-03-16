#define _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "pcg/pcg_basic.h"

#include <vector>
#include <array>
#include <random>
#include <direct.h>
#include <chrono>

#define DETERMINISTIC() false

static const int c_numPointsPerTest = 1000;
static const int c_numTests = 100;
static const int c_imageSize = 256;


static const float c_oneOverE = 1.0f / std::exp(1.0f);

typedef std::array<float, 2> float2;

pcg32_random_t GetRNG()
{
	pcg32_random_t rng;

#if DETERMINISTIC()
	pcg32_srandom_r(&rng, 0x1337b337, 0xbeefcafe);
#else
	std::random_device rd;
	pcg32_srandom_r(&rng, rd(), rd());
#endif

	return rng;
}

float RandomFloat01(pcg32_random_t& rng)
{
	return ldexpf((float)pcg32_random_r(&rng), -32);
}

float Distance(const float2& A, const float2& B)
{
	float dx = A[0] - B[0];
	float dy = A[1] - B[1];
	return std::sqrt(dx * dx + dy * dy);
}

double Lerp(double A, double B, double t)
{
	return A * (1.0 - t) + B * t;
}

std::vector<float2> MitchellsBestCandidate(int count, int& hotLoopCount)
{
	std::vector<float2> ret(count);

	pcg32_random_t rng = GetRNG();

	hotLoopCount = 0;

	// for each point we want to generate
	for (int newPointIndex = 0; newPointIndex < count; ++newPointIndex)
	{
		// generate the current number of points + 1 candidates.
		int candidateCount = newPointIndex + 1;

		// for each candidate
		float2 bestCandidate = {};
		float bestCandidateScore = 0.0f;
		for (int candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
		{
			// generate a random position
			float2 candidate = { RandomFloat01(rng), RandomFloat01(rng) };
			float candidateScore = FLT_MAX;

			// the score is the distance to the closest existing point
			for (int pointIndex = 0; pointIndex < newPointIndex; ++pointIndex)
			{
				candidateScore = std::min(candidateScore, Distance(candidate, ret[pointIndex]));
				hotLoopCount++;
			}

			// the best candidate is the candidate with the largest score
			if (candidateScore > bestCandidateScore)
			{
				bestCandidateScore = candidateScore;
				bestCandidate = candidate;
			}
		}

		// keep the best candidate
		ret[newPointIndex] = bestCandidate;
	}

	// return the points
	return ret;
}

std::vector<float2> EulersBestCandidate(int count, int& hotLoopCount)
{
	std::vector<float2> ret(count);

	pcg32_random_t rng = GetRNG();

	hotLoopCount = 0;

	// for each point we want to generate
	for (int newPointIndex = 0; newPointIndex < count; ++newPointIndex)
	{
		// generate the current number of points + 1 candidates.
		int candidateCount = newPointIndex + 1;

		int minimumCandidateIndexExit = (int)std::ceil(float(candidateCount) * c_oneOverE);

		// for each candidate
		float2 bestCandidate = {};
		float bestCandidateScore = 0.0f;
		for (int candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
		{
			// generate a random position
			float2 candidate = { RandomFloat01(rng), RandomFloat01(rng) };
			float candidateScore = FLT_MAX;

			// the score is the distance to the closest existing point
			for (int pointIndex = 0; pointIndex < newPointIndex; ++pointIndex)
			{
				candidateScore = std::min(candidateScore, Distance(candidate, ret[pointIndex]));
				hotLoopCount++;
			}

			// the best candidate is the candidate with the largest score
			if (candidateScore > bestCandidateScore)
			{
				bestCandidateScore = candidateScore;
				bestCandidate = candidate;

				if (candidateIndex >= minimumCandidateIndexExit)
					break;
			}
		}

		// keep the best candidate
		ret[newPointIndex] = bestCandidate;
	}

	// return the points
	return ret;
}

void SaveImage(const std::vector<float2>& points, const char* baseFileName, int fileIndex, int imageSize)
{
	// make the image
	std::vector<unsigned char> pixels(imageSize * imageSize, 255);
	for (const float2& point : points)
	{
		int px = std::min(int(point[0] * float(imageSize)), imageSize - 1);
		int py = std::min(int(point[1] * float(imageSize)), imageSize - 1);
		pixels[py * imageSize + px] = 0;
	}

	// save the image
	char fileName[1024];
	sprintf_s(fileName, "%s%i.png", baseFileName, fileIndex);
	stbi_write_png(fileName, imageSize, imageSize, 1, pixels.data(), 0);
}

int main(int argc, char** argv)
{
	_mkdir("out");

	// MBC
	double MBCMean = 0.0;
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		int hotLoopCount = 0;

		int lastPercent = -1;
		for (int testIndex = 0; testIndex < c_numTests; ++testIndex)
		{
			int percent = int(100.0f * float(testIndex) / float(c_numTests));
			if (lastPercent != percent)
				printf("\rMBC: %i%%", percent);

			std::vector<float2> points = MitchellsBestCandidate(c_numPointsPerTest, hotLoopCount);
			SaveImage(points, "out/MBC_", testIndex, c_imageSize);
		}
		printf("\rMBC: 100%%\n");

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		float seconds = float(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f;
		printf("hotLoops = %0.0f\n", double(hotLoopCount));
		printf("Generated in %0.4f seconds\n", seconds);
		MBCMean = double(hotLoopCount);
	}

	printf("\n");

	// EBC
	double EBCMean = 0.0;
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		double hotLoopsAvg = 0.0;
		double hotLoopsSqAvg = 0.0;

		int lastPercent = -1;
		for (int testIndex = 0; testIndex < c_numTests; ++testIndex)
		{
			int percent = int(100.0f * float(testIndex) / float(c_numTests));
			if (lastPercent != percent)
				printf("\rEBC: %i%%", percent);

			int hotLoopCount = 0;
			std::vector<float2> points = EulersBestCandidate(c_numPointsPerTest, hotLoopCount);
			SaveImage(points, "out/EBC_", testIndex, c_imageSize);

			hotLoopsAvg = Lerp(hotLoopsAvg, double(hotLoopCount), 1.0f / double(testIndex + 1));
			hotLoopsSqAvg = Lerp(hotLoopsSqAvg, double(hotLoopCount) * double(hotLoopCount), 1.0 / double(testIndex + 1));
		}
		printf("\rEBC: 100%%\n");

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		float seconds = float(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()) / 1000.0f;
		printf("hotLoops = %0.0f (std. dev. %f)\n", hotLoopsAvg, std::sqrt(hotLoopsSqAvg - hotLoopsAvg * hotLoopsAvg));
		printf("Generated in %0.4f seconds\n", seconds);
		EBCMean = hotLoopsAvg;
	}

	printf("\n");

	printf("EBC hotloops: %0.2f%%\n\n", 100.0f * EBCMean / MBCMean);

	// make the expected DFTs!
	printf("making DFTs...\n\n");
	char buffer[1024];
	sprintf_s(buffer, "python MakeExpectedDFT.py out/MBC_ %i out/_MBC.png", c_numTests);
	system(buffer);

	sprintf_s(buffer, "python MakeExpectedDFT.py out/EBC_ %i out/_EBC.png", c_numTests);
	system(buffer);

	return 0;
}

/*

TODO:
* could this have uses in reservoir sampling?
* could show or explain how time increases with number of samples.
* could have another usage case, choosing candidates to fill out a texture or something. maybe making a blue noise texture?

Note:
* 73% of the candidates to test is nice.
* variance is high though.

*/
