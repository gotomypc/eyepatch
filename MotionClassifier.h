#pragma once
#include "Classifier.h"

class MotionClassifier : public Classifier {
public:
    MotionClassifier();
    MotionClassifier(LPCWSTR pathname);
    ~MotionClassifier();

    BOOL ContainsSufficientSamples(TrainingSet*);
	void StartTraining(TrainingSet*);
	void ClassifyFrame(IplImage*, IplImage*);
    void ClassifyMotion(IplImage*, double, IplImage*);
    void Save();

private:
    list<double> motionAngles;
};