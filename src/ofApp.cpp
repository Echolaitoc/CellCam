#include "ofApp.h"
#include "ofUtils.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetWindowTitle("VoronoiCam");
    ofBackground(BACKGROUND_COLOR);

	blobActionAreaSum = 0;
	blobActionTimer = 0;

	drawCam = false;
	drawVoronoi = false;
	learnBackground = true;

	camWidth = 1280;
	camHeight = 720;
	cam.setup(1280, 720);
	camMirror = new unsigned char[camWidth*camHeight * 3];
	//mirrorTexture.allocate(camWidth, camHeight, GL_RGB);
	cvImage.allocate(camWidth, camHeight);
	cvBackground.allocate(camWidth, camHeight);
	oldCvGrayImage.allocate(camWidth, camHeight);
	cvGrayImage.allocate(camWidth, camHeight);
	cvGrayDiff.allocate(camWidth, camHeight);
	cvThreshold = 50;

    ofRectangle bounds = ofRectangle(-50, -50, ofGetWidth() + 100, ofGetHeight() + 100);
    int pointCount = 255;
	int relaxationSteps = 10;
	int seed = ofRandom(ofGetUnixTime());
    
	setupVoronoi(pointCount, seed, relaxationSteps, bounds, false);
}

//--------------------------------------------------------------
void ofApp::setupVoronoi(int pointCount, int seed, int relaxationSteps, ofRectangle bounds, bool beehive)
{
	if (beehive) {
		points = generateBeehivePoints(ofPoint(pointCount / 16, pointCount / 9), ofPoint(0, 30), seed, bounds);
	}
	else {
		points = generateRandomPoints(pointCount, seed, bounds);
	}

	voronoi.setBounds(bounds);
	voronoi.setPoints(points);

	voronoi.generate();
	for (int i = 0; i < relaxationSteps; ++i) {
		voronoi.relax();
	}

	points.clear();
	vector<ofxVoronoiCell> baseCells;
	for (auto cell : voronoi.getCells()) {
		points.push_back(cell.pt);
		baseCells.push_back(cell);
	}

	for (auto cell : baseCells) {
		float length = 0;
		for (int i = 0; i < cell.pts.size(); ++i) {
			auto point = cell.pts.at(i);
			auto next = cell.pts.at((i + 1) == cell.pts.size() ? 0 : i + 1);
			length += point.distance(next);
		}
		vector <ofPoint> reducedPoints;
		for (int i = 0; i < cell.pts.size(); ++i) {
			auto point = cell.pts.at(i);
			auto next = cell.pts.at((i + 1) == cell.pts.size() ? 0 : i + 1);
			if (point.distance(next) > length / 20) {
				reducedPoints.push_back(point);
			}
		}

		if (reducedPoints.size() >= 3) {
			cells.push_back(cell);
			cells.at(cells.size() - 1).pts.clear();
			cells.at(cells.size() - 1).pts = reducedPoints;
		}
	}

	shapes = generateShapes(cells);
}

//--------------------------------------------------------------
void ofApp::update()
{
	blobActionTimer -= ofGetLastFrameTime();
	if (blobActionTimer <= 0) {
		blobActionTimer = MIN_BLOB_ACTION_TIME;
		blobActionAreaSum = 0;
		learnBackground = true;
	}
	updateCvBackground();
	mirrorCam();
	updateContourBlobs();
	floatPoints(false);
	updateAnimShapeVisibility();
}

//--------------------------------------------------------------
void ofApp::updateCvBackground()
{
	if (learnBackground) {
		cvBackground = oldCvGrayImage;
		learnBackground = false;
	}
	oldCvGrayImage = cvGrayImage;
}

//--------------------------------------------------------------
void ofApp::mirrorCam()
{
	cam.update();
	if (cam.isFrameNew()) {
		unsigned char * pixels = cam.getPixels();
		for (int i = 0; i < camHeight; i++) {
			for (int j = 0; j < camWidth * 3; j += 3) {
				// pixel number
				int pix1 = (i*camWidth * 3) + j;
				int pix2 = (i*camWidth * 3) + (j + 1);
				int pix3 = (i*camWidth * 3) + (j + 2);
				// mirror pixel number
				int mir1 = (i*camWidth * 3) + 1 * (camWidth * 3 - j - 3);
				int mir2 = (i*camWidth * 3) + 1 * (camWidth * 3 - j - 2);
				int mir3 = (i*camWidth * 3) + 1 * (camWidth * 3 - j - 1);
				// swap pixels
				camMirror[pix1] = pixels[mir1];
				camMirror[pix2] = pixels[mir2];
				camMirror[pix3] = pixels[mir3];
			}
		}
		cvImage.setFromPixels(camMirror, camWidth, camHeight);
	}
}

void ofApp::updateContourBlobs()
{
	cvGrayImage = cvImage;
	cvGrayDiff.absDiff(oldCvGrayImage, cvGrayImage);
	cvGrayDiff.threshold(cvThreshold);
	contourFinder.findContours(cvGrayDiff, 20, (camWidth * camHeight) / 3, 10, false);
	for (auto blob : contourFinder.blobs) {
		blobActionAreaSum += blob.area;
	}
	if (blobActionAreaSum >= MAX_BLOB_AREA_ACTION * MIN_BLOB_ACTION_TIME) {
		blobActionTimer = MIN_BLOB_ACTION_TIME;
		blobActionAreaSum = 0;
	}
	cvGrayDiff.absDiff(cvBackground, cvGrayImage);
	cvGrayDiff.threshold(cvThreshold);
	contourFinder.findContours(cvGrayDiff, 20, (camWidth * camHeight) / 3, 10, false);
}

void ofApp::updateAnimShapeVisibility() {
	for (auto& animShape : shapes) {
		bool visible = true;
		int animShapePositionX = (animShape.center.x / ofGetWidth()) * camWidth;
		int animShapePositionY = (animShape.center.y / ofGetHeight()) * camHeight;
		if (!ofRectangle(0, 0, cvGrayDiff.width, cvGrayDiff.height).inside(animShapePositionX, animShapePositionY)) {
			continue;
		}
		int pixelBrightness = cvGrayDiff.getPixels()[animShapePositionY * cvGrayDiff.width + animShapePositionX];
		if (pixelBrightness > 0) {
			visible = false;
		}
		animShape.visible = visible;
		if (visible) {
			animShape.shape.setColor(SHAPE_COLOR_NORMAL);
		}
		else {
			animShape.shape.setColor(SHAPE_COLOR_ACTIVE);
		}
	}
}

//--------------------------------------------------------------
bool ofApp::pointInPolygon(int pno, int x, int y)
{
	int i, j = contourFinder.blobs[pno].pts.size() - 1;
	bool  oddNodes = false;

	for (i = 0; i<contourFinder.blobs[pno].pts.size(); i++) {
		if (contourFinder.blobs[pno].pts[i].y<y && contourFinder.blobs[pno].pts[j].y >= y
			|| contourFinder.blobs[pno].pts[j].y<y && contourFinder.blobs[pno].pts[i].y >= y) {
			if (contourFinder.blobs[pno].pts[i].x + (y - contourFinder.blobs[pno].pts[i].y) / (contourFinder.blobs[pno].pts[j].y - contourFinder.blobs[pno].pts[i].y)*(contourFinder.blobs[pno].pts[j].x - contourFinder.blobs[pno].pts[i].x)<x) {
				oddNodes = !oddNodes;
			}
		}
		j = i;
	}

	return oddNodes;
}

//--------------------------------------------------------------
void ofApp::floatPoints(bool regenVoronoi)
{
	if (regenVoronoi) {
		vector <ofPoint> newPoints;
		for (auto point : points) {
			newPoints.push_back(ofPoint(point));
			newPoints.at(newPoints.size() - 1).x = point.x + sin(point.y + ofGetFrameNum() * 0.1f) * 3;
			newPoints.at(newPoints.size() - 1).y = point.y + cos(point.x + ofGetFrameNum() * 0.1f) * 3;
		}
		voronoi.clear();
		voronoi.setPoints(newPoints);
		voronoi.generate();
		shapes = generateShapes(voronoi.getCells());
	}
	else {
		vector <ofxVoronoiCell> newCells;
		for (auto cell : cells) {
			newCells.push_back(cell);
			newCells.at(newCells.size() - 1).pt = getNewFloatyPointPosition(cell.pt, 0.05f, 7);
			for (int i = 0; i < cell.pts.size(); ++i) {
				newCells.at(newCells.size() - 1).pts.at(i) = getNewFloatyPointPosition(cell.pts.at(i), 0.2f, 7);
			}
		}
		shapes = generateShapes(newCells);
	}
}

//--------------------------------------------------------------
ofPoint ofApp::getNewFloatyPointPosition(ofPoint basePosition, float speed, float distance)
{
	ofPoint newPosition(0, 0);
	newPosition.x = basePosition.x + sin(basePosition.y + ofGetFrameNum() * speed) * distance;
	newPosition.y = basePosition.y + cos(basePosition.x + ofGetFrameNum() * speed) * distance;
	return newPosition;
}

//--------------------------------------------------------------
void ofApp::draw()
{
	if (drawVoronoi) {
		voronoi.draw();
	}
	for(auto& shape : shapes) {
		//if (shape.visible) {
			shape.shape.draw();
		//}
	}
	if (drawCam) {
		cvImage.draw(0, 0, ofGetWidth(), ofGetHeight());
		cvGrayDiff.draw(0, 0, ofGetWidth(), ofGetHeight());
		contourFinder.draw(0, 0, ofGetWidth(), ofGetHeight());
		//for (int i = 0; i < contourFinder.blobs.size(); ++i) {
		//	contourFinder.blobs.at(i).draw();
		//	//ofDrawRectangle(contourFinder.blobs.at(i).boundingRect);
		//}
		stringstream str;
		str << "Threshold: " << cvThreshold;
		ofDrawBitmapString(str.str(), 100, 100);
	}
}

//--------------------------------------------------------------
vector <ofPoint> ofApp::generateRandomPoints(int count, int seed, ofRectangle bounds)
{
    vector <ofPoint> points;
    ofSeedRandom(seed);
    
    for(int i=0; i<count; ++i) {
        ofPoint newPoint = ofPoint(
            ofRandom(bounds.x, bounds.width),
            ofRandom(bounds.y, bounds.height)
        );
        
        points.push_back(newPoint);
    }
    
    return points;
}

//--------------------------------------------------------------
vector <ofPoint> ofApp::generateBeehivePoints(ofPoint size, ofPoint variance, int seed, ofRectangle bounds)
{
	vector <ofPoint> points;
	ofSeedRandom(seed);

	ofPoint col = ofPoint(floorf(bounds.width / size.x), floorf(bounds.height / size.y));
	cout << col << endl;
	for (int i = -1; i < size.x + 1; ++i) {
		for (int j = -1; j < size.y + 1; ++j) {
			ofPoint point = ofPoint(i, j) / ofPoint(size.x, size.y) * ofPoint(bounds.width, bounds.height) + col / 2;
			if (j % 2) {
				point += ofPoint(col.x / 2, 0);
			}
			ofPoint rndPoint = ofPoint(
				ofRandom(variance.x, variance.y),
				ofRandom(variance.x, variance.y)
				);
			point += rndPoint;
			points.push_back(point);
		}
	}
	return points;
}

//--------------------------------------------------------------
vector <ofApp::animShape> ofApp::generateShapes(vector<ofxVoronoiCell> cells)
{
	this->shapes.clear();
	vector <animShape> shapes;
	for (auto cell : cells) {
		shapes.push_back(generateShape(cell.pts, cell.pt));
	}
	return shapes;
}

//--------------------------------------------------------------
ofApp::animShape ofApp::generateShape(vector <ofPoint> points, ofPoint center, bool reducePoints)
{
	animShape shape;

	vector <ofPoint> reducedPoints;
	if (reducePoints) {
		float length = 0;
		for (int i = 0; i < points.size(); ++i) {
			auto point = points.at(i);
			auto next = points.at((i + 1) == points.size() ? 0 : i + 1);
			length += point.distance(next);
		}
		for (int i = 0; i < points.size(); ++i) {
			auto point = points.at(i);
			auto next = points.at((i + 1) == points.size() ? 0 : i + 1);
			if (point.distance(next) > length / 20) {
				reducedPoints.push_back(point);
			}
		}
		if (reducedPoints.size() < 3) {
			return shape;
		}
	}
	else {
		reducedPoints = points;
	}

	for (int i = 0; i < reducedPoints.size(); ++i) {
		auto previous = reducedPoints.at((i - 1) < 0 ? reducedPoints.size() - 1 : i - 1);
		auto point = reducedPoints.at(i);
		auto next = reducedPoints.at((i + 1) == reducedPoints.size() ? 0 : i + 1);
		auto nextNext = reducedPoints.at((i + 2) == reducedPoints.size() ? 0 : (i + 1) == reducedPoints.size() ? 1 : i + 2);
		
		float smoothing = 0.3f;

		auto tangentA = (previous - next).getNormalized();
		auto handleA = point - tangentA * point.distance(next) * smoothing;

		auto tangentB = (point - nextNext).getNormalized();
		auto handleB = next + tangentB * point.distance(next) * smoothing;
		
		if (i == 0) {
			shape.shape.moveTo(point);
		}
		shape.shape.bezierTo(handleA, handleB, next);
	}
	float scaleFactor = 0.75f;
	shape.shape.scale(scaleFactor, scaleFactor);
	shape.shape.translate(center - center * scaleFactor);

	shape.shape.setFillColor(SHAPE_COLOR_NORMAL);

	shape.cornerPoints = reducedPoints;
	shape.center = center;
	shape.visible = true;
	return shape;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	switch (key) {
	case 'c':
	case 'C':
		drawCam = !drawCam;
		break;
	case 'v':
	case 'V':
		drawVoronoi = !drawVoronoi;
		break;
	case '+':
		++cvThreshold;
		break;
	case '-':
		--cvThreshold;
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
