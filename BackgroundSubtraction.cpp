#include "precomp.h"
#include "precomp.h"
#include "constants.h"
#include "TrainingSample.h"
#include "TrainingSet.h"
#include "Classifier.h"
#include "BackgroundSubtraction.h"

BackgroundSubtraction::BackgroundSubtraction() :
	Classifier() {

    // set the "friendly name" and type
    wcscpy(friendlyName, L"Adaptive Background Subtraction");
    classifierType = FILTER_BUILTIN;

    isTrained = false;
    isOnDisk = false;

    frameNum = 0;
    bgmodel = NULL;


	smallFrameCopy = cvCreateImage(cvSize(160, 120), IPL_DEPTH_8U, 3);
	fgMaskSmall = cvCreateImage(cvSize(160, 120), IPL_DEPTH_8U, 1);
}

BackgroundSubtraction::BackgroundSubtraction(LPCWSTR pathname) :
	Classifier() {

    // We will never load standard filters from disk, so this should never be called
    assert(FALSE);
}

BackgroundSubtraction::~BackgroundSubtraction() {
	cvReleaseImage(&smallFrameCopy);
	if (isTrained) {
		cvReleaseBGStatModel(&bgmodel);
		cvReleaseImage(&fgMask);
	}
}

BOOL BackgroundSubtraction::ContainsSufficientSamples(TrainingSet *sampleSet) {

    // Standard filters don't need training samples
    assert(FALSE);
    return true;
}

void BackgroundSubtraction::StartTraining(TrainingSet *sampleSet) {

    // Standard filters don't use training (this should never be called)
    assert(FALSE);
}

void BackgroundSubtraction::ClassifyFrame(IplImage *frame, IplImage* guessMask) {
    if(!frame) return;

	cvResize(frame, smallFrameCopy);

    if (!isTrained) {
		
        // we need some more background frames before we have a decent background model
        if (frameNum < BACKGROUND_SUBTRACTION_DISCARD_FRAMES) {
            // throw away the first few frames (usually junk)
        } else if (frameNum == BACKGROUND_SUBTRACTION_DISCARD_FRAMES) {
            // initialize the background model on this frame
			CvFGDStatModelParams bgmodelparams = {CV_BGFG_FGD_LC, CV_BGFG_FGD_N1C, CV_BGFG_FGD_N2C, CV_BGFG_FGD_LCC,
				CV_BGFG_FGD_N1CC, CV_BGFG_FGD_N2CC, 1, 1, 2*CV_BGFG_FGD_ALPHA_1, 3*CV_BGFG_FGD_ALPHA_2,
				2*CV_BGFG_FGD_ALPHA_3, CV_BGFG_FGD_DELTA, CV_BGFG_FGD_T, CV_BGFG_FGD_MINAREA};
            bgmodel = cvCreateFGDStatModel(smallFrameCopy, &bgmodelparams);
			fgMask = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 1);
		} else {
            // update background model
            cvUpdateBGStatModel(smallFrameCopy, bgmodel);
        }
        frameNum++;

        // see if we're done training
        if (frameNum >= BACKGROUND_SUBTRACTION_DISCARD_FRAMES + BACKGROUND_SUBTRACTION_MIN_FRAMES) {
            isTrained = true;
        }
    } else {

        // update background model
        cvUpdateBGStatModel(smallFrameCopy, bgmodel);

        // close the foreground mask
		cvCopy(bgmodel->foreground, fgMaskSmall);
		IplConvKernel *circElem = cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_ELLIPSE);        
		cvMorphologyEx(fgMaskSmall, fgMaskSmall, 0, circElem, CV_MOP_CLOSE, 3);
		cvReleaseStructuringElement(&circElem);

        // Combine foreground mask with passed-in mask
		cvResize(fgMaskSmall, fgMask);
        cvAnd(guessMask, fgMask, guessMask);
    }
}


void BackgroundSubtraction::Save() {
    
    // We can't save built-in classifiers, so this should never be called
    assert(FALSE);
}

void BackgroundSubtraction::ResetRunningState() {
	if (isTrained) {
		cvReleaseBGStatModel(&bgmodel);
		cvReleaseImage(&fgMask);
	}
	isTrained = false;
	frameNum = 0;
}