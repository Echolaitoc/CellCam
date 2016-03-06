#pragma once

#include "ofMain.h"
#include "ofxVoronoi.h"
#include "ofxOpenCv.h"

class ofApp : public ofBaseApp{
    private:
		struct animShape {
			vector<ofPoint> cornerPoints;
			ofPoint center;
			ofPath shape;
			bool visible;
		};

        vector <ofPoint> generateRandomPoints(int count, int seed, ofRectangle bounds);
		vector <ofPoint> generateBeehivePoints(ofPoint size, ofPoint variance, int seed, ofRectangle bounds);
		vector <animShape> generateShapes(vector<ofxVoronoiCell> cells);
		animShape generateShape(vector <ofPoint> points, ofPoint center, bool reducePoints = false);
		ofPoint getNewFloatyPointPosition(ofPoint basePosition, float speed, float distance);
		bool pointInPolygon(int pno, int x, int y);
		void setupVoronoi(int pointCount, int seed, int relaxationSteps, ofRectangle bounds, bool beehive);
		void floatPoints(bool regenVoronoi);
		void updateCvBackground();
		void mirrorCam();
		void updateContourBlobs();
		void updateAnimShapeVisibility();

        ofxVoronoi voronoi;
        vector<ofPoint> points;
		vector<ofxVoronoiCell> cells;
		vector<animShape> shapes;
		ofVideoGrabber cam;
		//ofTexture mirrorTexture;
		ofxCvColorImage cvImage;
		ofxCvGrayscaleImage cvBackground;
		ofxCvGrayscaleImage oldCvGrayImage;
		ofxCvGrayscaleImage cvGrayImage;
		ofxCvGrayscaleImage cvGrayDiff;
		ofxCvContourFinder contourFinder;
		unsigned char * camMirror;
		float blobActionTimer;
		float blobActionAreaSum;
		int camWidth;
		int camHeight;
		int cvThreshold;
		bool drawCam;
		bool drawVoronoi;
		bool learnBackground;
		const ofColor BACKGROUND_COLOR = ofColor(255, 200, 50);
		const ofColor SHAPE_COLOR_NORMAL = ofColor(20, 155, 95);
		const ofColor SHAPE_COLOR_ACTIVE = ofColor(15, 105, 175);
		const float MAX_BLOB_AREA_ACTION = 1000;
		const float MIN_BLOB_ACTION_TIME = 5;

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
};
