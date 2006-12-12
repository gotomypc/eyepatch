#include "precomp.h"
#include "TrainingSample.h"
#include "TrainingSet.h"
#include "Classifier.h"
#include "GestureClassifier.h"

GestureClassifier::GestureClassifier() :
	Classifier() {
    nModels = 0;
    models = NULL;
}

GestureClassifier::~GestureClassifier() {
    if (isTrained) { // delete the models
        for (int i=0; i<nModels; i++) {
            delete models[i];
        }
        delete[] models;
    }
}

void GestureClassifier::StartTraining(TrainingSet *sampleSet) {
    if (isTrained) { // delete the old models
        for (int i=0; i<nModels; i++) {
            delete models[i];
        }
        delete[] models;
    }

    nModels = sampleSet->rangeSampleCount;
    models = new TrajectoryModel*[nModels];
    int modelNum = 0;
    cvZero(filterImage);
    IplImage *resizedGestureImage = cvCloneImage(filterImage);

    // TODO: call into trainingset class to do this instead of accessing samplemap
    for (map<UINT, TrainingSample*>::iterator i = sampleSet->sampleMap.begin(); i != sampleSet->sampleMap.end(); i++) {
        TrainingSample *sample = (*i).second;
        if (sample->iGroupId == GROUPID_RANGESAMPLES) { // gesture (range) sample
            IplImage *gestureImage = cvCreateImage(cvSize(sample->fullImageCopy->width, sample->fullImageCopy->height), IPL_DEPTH_8U, 3);
            cvZero(gestureImage);
            DrawTrack(gestureImage, sample->motionTrack, colorSwatch[modelNum % COLOR_SWATCH_SIZE], 3);
            cvResize(gestureImage, resizedGestureImage);
            cvAdd(filterImage, resizedGestureImage, filterImage);
            models[modelNum] = new TrajectoryModel(sample->motionTrack);
            modelNum++;
		}
    }

    // update demo image
    IplToBitmap(filterImage, filterBitmap);
    cvReleaseImage(&resizedGestureImage);

    // update member variables
	isTrained = true;
}

BOOL GestureClassifier::ContainsSufficientSamples(TrainingSet *sampleSet) {
    return (sampleSet->rangeSampleCount > 0);
}

void GestureClassifier::ClassifyFrame(IplImage *frame, list<Rect>* objList) {
    if (!isTrained) return;
    if(!frame) return;
    objList->clear();
}    

void GestureClassifier::ClassifyTrack(MotionTrack mt, list<Rect>* objList) {

    objList->clear();
    CondensationSampleSet condensSampleSet(GESTURE_NUM_CONDENSATION_SAMPLES, models, nModels);

    // allocate storage for graph points (one curve for each model to show its match probability)
    int nCurvePts = mt.size();
    CvPoint **curvePts = new CvPoint*[nModels];
    for (int i=0; i<nModels; i++) {
        curvePts[i] = new CvPoint[nCurvePts];
    }

    // initialize graph size values
    double xStep = ((double)applyImage->width) / ((double)mt.size());
    double yMax = (double)applyImage->height;
    double xPos = 0.0;

    MotionSample ms;
    for (int i = 0; i<mt.size(); i++) {
        // update probabilities based on this sample point
        ms = mt[i];
        condensSampleSet.Update(ms.vx, ms.vy, ms.sizex, ms.sizey);

        for (int modelNum=0; modelNum<nModels; modelNum++) {
            double probability = condensSampleSet.GetModelProbability(modelNum);
            double completionProb = condensSampleSet.GetModelCompletionProbability(modelNum);
            curvePts[modelNum][i] = cvPoint(xPos, probability*yMax);
        }
        xPos += xStep;
    }
    cvZero(applyImage);
    for (int modelNum=0; modelNum<nModels; modelNum++) {
        cvPolyLine(applyImage, &(curvePts[modelNum]), &nCurvePts, 1, 0, colorSwatch[modelNum % COLOR_SWATCH_SIZE], 2, CV_AA);
        delete[] curvePts[modelNum];
    }
    delete[] curvePts;
    IplToBitmap(applyImage, applyBitmap);

    for (int modelNum=0; modelNum<nModels; modelNum++) {
        double probability = condensSampleSet.GetModelProbability(modelNum);
        double completionProb = condensSampleSet.GetModelCompletionProbability(modelNum);
        if (completionProb>0.1) {
            Rect objRect;
            objRect.X = ms.x - ms.sizex/2;
            objRect.Y = ms.y - ms.sizey/2;
            objRect.Width = ms.sizex;
            objRect.Height = ms.sizey;
            objList->push_back(objRect);
        }
    }
}