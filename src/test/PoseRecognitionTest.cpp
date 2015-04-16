#include "../PoseRecognition.cpp"
using namespace iez;
	cv::Point Processing::pointMean(const cv::Point &pt1, const cv::Point &pt2, const float ratio12)
	{
		return cv::Point((pt1.x*ratio12 + pt2.x*(1 - ratio12)), (pt1.y*ratio12 + pt2.y*(1 - ratio12)));
	}

#include <gtest/gtest.h>

class PoseRecognitionTest : public testing::Test
{
	void SetUp() {
		srand(time(NULL));
	}

	void TearDown() {

	}


protected:
	PoseRecognition *pose;
};

TEST_F(PoseRecognitionTest, integrityTest)
{
	int j = 100;
	QList<PoseRecognition::Data> db;

	for (int m = 0; m < j; m++) {
		PoseRecognition::Data dataSource(INPUT_VECTOR_SIZE);
		for (int i = 0; i < INPUT_VECTOR_SIZE; i++) {
			dataSource.input[i] = 100000.0f/rand()*((rand()%2) ? -1 : 1);
		}
		dataSource.output = rand()%100;

		db.append(dataSource);
	}

	PoseRecognition::saveDatabaseToFile("/tmp/k.csv", db);
	QList<PoseRecognition::Data> dbResult = PoseRecognition::loadDatabaseFromFile("/tmp/k.csv");

	EXPECT_EQ(dbResult.size(), db.size());
	for (int m = 0; m < j; m++) {
		for (int i = 0; i < INPUT_VECTOR_SIZE; i++) {
			EXPECT_FLOAT_EQ(QString::number(db[m].input[i]).toFloat(), QString::number(dbResult[m].input[i]).toFloat());
		}
		EXPECT_EQ(db[m].output, dbResult[m].output);
	}


}

