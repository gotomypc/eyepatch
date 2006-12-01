#include "precomp.h"
#include "_cvhaartraining.h"
#include "TrainingSample.h"
#include "TrainingSet.h"
#include "Classifier.h"
#include "HaarClassifier.h"

HaarClassifierDialog::HaarClassifierDialog(HaarClassifier *p) {
	parent = p;
	m_hThread = NULL;
}

HaarClassifierDialog::~HaarClassifierDialog() {
}

LRESULT HaarClassifierDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    CenterWindow();
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadCallback, (LPVOID)this, 0, &threadID);
	return TRUE;    // let the system set the focus
}

LRESULT HaarClassifierDialog::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	TerminateThread(m_hThread, 0);
	parent->isTrained = false;
	if (parent->nStagesCompleted >= MIN_HAAR_STAGES) {
		parent->cascade = cvLoadHaarClassifierCascade(parent->classifierName, cvSize(SAMPLE_X, SAMPLE_Y));
		if (parent->cascade != NULL) {
			parent->isTrained = true;
		}
	}
	return 0;
}

DWORD WINAPI HaarClassifierDialog::ThreadCallback(HaarClassifierDialog* instance) {
	instance->Train();
	return 1L;
}

void HaarClassifierDialog::Train() {
    cvCreateCascadeClassifier(parent->classifierPathname,  parent->vecFilename, parent->negFilename, 
        parent->nPosSamples, parent->nNegSamples, parent->nStages,
		0, 2, .99, .5, .95, 0, 1, 1, SAMPLE_X, SAMPLE_Y, 3, 0,
		GetDlgItem(IDC_HAAR_PROGRESS), &(parent->nStagesCompleted));
	::EndDialog(m_hWnd, IDOK);
}


HaarClassifier::HaarClassifier() :
	Classifier(),
	m_progressDlg(this) {
    cascade = NULL;
    nStages = START_HAAR_STAGES;
    storage = cvCreateMemStorage(0);
	nStagesCompleted = 0;
}

HaarClassifier::~HaarClassifier() {
    cvReleaseMemStorage(&storage);
    if (isTrained) cvReleaseHaarClassifierCascade(&cascade);
}

void HaarClassifier::PrepareData(TrainingSet *sampleSet) {

    int gridSize = (int) ceil(sqrt((double)sampleSet->posSampleCount));
    int gridX = 0;
    int gridY = 0;
    int gridSampleW = FILTERIMAGE_WIDTH / gridSize;
    int gridSampleH = FILTERIMAGE_HEIGHT / gridSize;
    cvZero(filterImage);

    char tempPathname[MAX_PATH];
    char imageFilename[MAX_PATH];

    GetTempPathA(MAX_PATH, tempPathname);
    sprintf_s(vecFilename, "%spossamples.vec", tempPathname);
    sprintf_s(negFilename, "%snegsamples.dat", tempPathname);
    int classifiernum = (int)time(0);
    sprintf_s(classifierPathname, "%sclassifier%d/", tempPathname, classifiernum);
    sprintf_s(classifierName, "%sclassifier%d", tempPathname, classifiernum);

    icvMkDir(vecFilename);
    icvMkDir(negFilename);
    FILE *vec = fopen(vecFilename, "wb");
    FILE *neglist = fopen(negFilename,"w");
    int imgNum=0;

    icvWriteVecHeader(vec, sampleSet->posSampleCount, SAMPLE_X, SAMPLE_Y);

    // TODO: call into trainingset class to do this instead of accessing samplemap
    for (map<UINT, TrainingSample*>::iterator i = sampleSet->sampleMap.begin(); i != sampleSet->sampleMap.end(); i++) {
        TrainingSample *sample = (*i).second;
        if (sample->iGroupId == 0) { // positive sample
            icvWriteVecSample(vec, sample->sampleImage);

            // draw sample into demo image
            IplImage *sampleCopyColor = cvCreateImage(cvSize(SAMPLE_X, SAMPLE_Y), IPL_DEPTH_8U, 3);
            cvCvtColor(sample->sampleImage, sampleCopyColor, CV_GRAY2BGR);
            CvMat *filterImageSubRect = cvCreateMat(gridSampleW, gridSampleH, CV_8UC1);
            cvGetSubRect(filterImage, filterImageSubRect, cvRect(gridX*gridSampleW,gridY*gridSampleH,gridSampleW,gridSampleH));
            cvResize(sampleCopyColor, filterImageSubRect);
            cvReleaseMat(&filterImageSubRect);
            cvReleaseImage(&sampleCopyColor);

            gridX++;
            if (gridX >= gridSize) {
                gridX = 0;
                gridY++;
            }

        } else if (sample->iGroupId == 1) { // negative sample
            sprintf_s(imageFilename, "%sneg%d.jpg", tempPathname, imgNum);
            cvSaveImage(imageFilename, sample->fullImageCopy);
            fprintf(neglist, "neg%d.jpg\n", imgNum);
            imgNum++;
        }
    }
    fclose(vec);
    fclose(neglist);
	nPosSamples = sampleSet->posSampleCount;
	nNegSamples = sampleSet->negSampleCount;

    // update demo image
    IplToBitmap(filterImage, filterBitmap);
}

void HaarClassifier::StartTraining(TrainingSet* sampleSet) {
	PrepareData(sampleSet);
	m_progressDlg.DoModal();
}

void HaarClassifier::ClassifyFrame(IplImage *frame, list<Rect>* objList) {
    if (!isTrained) return;
    if (!cascade) return;

    // Clear the memory storage we used before
    cvClearMemStorage( storage );

    // There can be more than one object in an image, so we create a growable sequence of objects
    // Detect the objects and store them in the sequence
    CvSeq* objects = cvHaarDetectObjects(frame, cascade, storage,
                                         1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
                                         cvSize(SAMPLE_X, SAMPLE_Y));

    IplImage *frameCopy = cvCreateImage(cvSize(frame->width,frame->height), IPL_DEPTH_8U, 3);
    cvCopy(frame, frameCopy);

    int objNum = 0;
    objList->clear();
    // Loop over the found objects
    for(int i = 0; i < (objects ? objects->total : 0); i++ )
    {
        Rect objRect;
        CvRect* r = (CvRect*)cvGetSeqElem(objects, i);
        cvRectangle(frameCopy, cvPoint(r->x,r->y), cvPoint(r->x+r->width,r->y+r->height), colorSwatch[objNum], 2, 8);
        objNum = (objNum + 1) % COLOR_SWATCH_SIZE;

        objRect.X = r->x;
        objRect.Y = r->y;
        objRect.Width = r->width;
        objRect.Height = r->height;
        objList->push_back(objRect);
    }

    cvResize(frameCopy, applyImage);
    IplToBitmap(applyImage, applyBitmap);
}