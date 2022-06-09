#include <cstdio>

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;

Timer timer;

bool crownCreated = false;
bool autoDeselect = false;

const float gravity = 0.01;
const double rateMax = 0.2;

const float C4 = 261.62;
const float Csharp4 = 277.18;
const float D4 = 293.67;
const float Dsharp4 = 311.31;
const float E4 = 329.63;
const float F4 = 349.23;
const float Fsharp4 = 369.99;
const float G4 = 392.00;
const float Gsharp4 = 415.30;
const float A4 = 440.00;
const float Asharp4 = 466.16;
const float B4 = 493.88;
const float C5 = 523.25;

std::vector<float> Notes {C4, Csharp4, D4, Dsharp4, E4, F4, Fsharp4, G4, Gsharp4, A4, Asharp4, B4, C5};

struct Piece {
    double baseX;
    double baseZ;
    double currentY;
    double targetY;
    double yRate;
    int reachedNewTarget;
    double hue;
    double size;
    bool selected;
    double octaveMod;
    void updatePiece();
    void setNewTarget(double target);
    void changeSize(double amount);
    void resetSize();
};

std::vector<Piece> Pieces {};
std::vector<Piece*> SortedPiecePointers {};

void Piece::updatePiece() {
        // std::cout << yRate << std::endl;
        if (reachedNewTarget == 0) {
            if (targetY > currentY) {
                yRate += gravity;
            }
            else if (targetY <= currentY) {
                yRate -= gravity;
            }
        }
        else if (reachedNewTarget == -1) {
            if (currentY > targetY) {
                reachedNewTarget = 0;
            }
            yRate += gravity;
        }
        else if (reachedNewTarget == 1) {
            if (currentY < targetY) {
                reachedNewTarget = 0;
            }
            yRate -= gravity;
        }
        if (yRate > rateMax) {
            yRate = rateMax;
        }
        else if (yRate < rateMax*-1) {
            yRate = rateMax*-1;
        }
        currentY += yRate;

        hue += 0.0005;
        if (hue > 1) {
            hue -= 1;
        }
}

void Piece::setNewTarget(double target) {
    if (target == targetY) {
        return;
    }
    else if (currentY < target) {
        reachedNewTarget = -1;
    }
    else {
        reachedNewTarget = 1;
    }
    targetY = target;
}

void Piece::changeSize(double amount) {
    if (size+amount <= 0) {
        return;
    }
    size += amount;
}

void Piece::resetSize() {
    size = 1;
}

void createSortedPointers() {
    for (int i = 0; i < Pieces.size(); i++) {
        Piece *newPointer = &Pieces[i];
        SortedPiecePointers.push_back(newPointer);
    }
    for (int i = 0; i < SortedPiecePointers.size()-1; i++) {
        for (int j = 0; j < SortedPiecePointers.size()-i-1; j++) {
            if (SortedPiecePointers[j]->baseZ < SortedPiecePointers[j+1]->baseZ) {
                Piece *temp = SortedPiecePointers[j];
                SortedPiecePointers[j] = SortedPiecePointers[j+1];
                SortedPiecePointers[j+1] = temp;
            }
        }
    }
}


class SquareWave : public SynthVoice
{
public:
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc7;

  gam::Env<3> mAmpEnv;

  void init() override
  {
    mAmpEnv.curve(0);
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2);

    createInternalTriggerParameter("amplitude", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  void onProcess(AudioIOData &io) override
  {
    float f = getInternalParameterValue("frequency");
    mOsc1.freq(f);
    mOsc3.freq(f * 3);
    mOsc5.freq(f * 5);
    mOsc7.freq(f * 7);

    float a = getInternalParameterValue("amplitude");
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      float s1 = mAmpEnv() * (mOsc1() * a +
                              mOsc3() * (a / 3.0) +
                              mOsc5() * (a / 5.0));

      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done())
      free();
  }

  void onProcess(Graphics& g) override {
  }

  void onTriggerOn() override { 
    mAmpEnv.reset(); 
    }

  void onTriggerOff() override { 
    mAmpEnv.release(); 
    }
};

std::vector<SquareWave*> Voices;

void changeVolume(int index, double amount) {
    double currentVolume = Voices[index]->getInternalParameterValue("amplitude");
    if (currentVolume+amount > 0) {
        Voices[index]->setInternalParameterValue("amplitude", currentVolume+amount);
    }
}

void modifyPieces(int position, float pitch) {
    for (size_t i = 0; i < Pieces.size(); i++) {
        if (Pieces[i].selected) {
            Pieces[i].setNewTarget(position);
            Voices[i]->setInternalParameterValue("frequency", Pieces[i].octaveMod*pitch);
            if (autoDeselect) {
                Pieces[i].selected = false;
            }           
        }
    }
}

void shiftPiecesDown() {
    for (size_t i = 0; i < Pieces.size(); i++) {
        if (Pieces[i].selected) {
            if (Pieces[i].targetY > -12) {
                Pieces[i].targetY -= 2;
            }
            for (size_t j = 0; j < Notes.size(); j++) {
                if (Voices[i]->getInternalParameterValue("frequency") == Pieces[i].octaveMod*Notes[j] && j > 0) {
                    Voices[i]->setInternalParameterValue("frequency", Pieces[i].octaveMod*Notes[j-1]);
                    break;
                }
            }
        }
    }
}

void shiftPiecesUp() {
    for (size_t i = 0; i < Pieces.size(); i++) {
        if (Pieces[i].selected) {
            if (Pieces[i].targetY < 12) {
                Pieces[i].targetY += 2;
            }
            for (size_t j = 0; j < Notes.size(); j++) {
                if (Voices[i]->getInternalParameterValue("frequency") == Pieces[i].octaveMod*Notes[j] && j < Notes.size()-1) {
                    Voices[i]->setInternalParameterValue("frequency", Pieces[i].octaveMod*Notes[j+1]);
                    break;
                }
            }
        }
    }
}

void modifyOctaves(double mod) {
    for (size_t i = 0; i < Pieces.size(); i++) {
        if (Pieces[i].selected) {
            Pieces[i].octaveMod *= mod;
            Voices[i]->setInternalParameterValue("frequency", mod*Voices[i]->getInternalParameterValue("frequency"));
        }
    }
}

void resetOctaves() {
    for (size_t i = 0; i < Pieces.size(); i++) {
        while (Pieces[i].octaveMod > 1) {
            Voices[i]->setInternalParameterValue("frequency", 0.5*Voices[i]->getInternalParameterValue("frequency"));
            Pieces[i].octaveMod *= 0.5;
        }
        while (Pieces[i].octaveMod < 1) {
            Voices[i]->setInternalParameterValue("frequency", 2*Voices[i]->getInternalParameterValue("frequency"));
            Pieces[i].octaveMod *= 2;
        }
    }
}

void scalePieces(int direction) {
    for (size_t i = 0; i < Pieces.size(); i++) {
        if (Pieces[i].selected) {
            Pieces[i].changeSize(0.15*direction);
            changeVolume(i, 0.01*direction);
        }
    }
}

void resetPieceSizes() {
    for (int i = 0; i < Pieces.size(); i++) {
        Pieces[i].resetSize();
        Voices[i]->setInternalParameterValue("amplitude", 0.05);
    }
}


class MyApp : public App
{
public:
  SynthGUIManager<SquareWave> synthManager{"SquareWave"};

  // Mesh mMesh;

  void onCreate() override
  {
    navControl().active(false); 

    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    synthManager.synthRecorder().verbose(true);

    // addSphere(mMesh, 1.0);
  }

  void onSound(AudioIOData &io) override
  {
    synthManager.render(io);
  }

  void onAnimate(double dt) override
  {
    imguiBeginFrame();
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  void onDraw(Graphics &g) override
  {
    g.clear();
    timer.stop();

    for (size_t i = 0; i < SortedPiecePointers.size(); i++) {
        g.pushMatrix();
        g.translate(SortedPiecePointers[i]->baseX, SortedPiecePointers[i]->currentY, SortedPiecePointers[i]->baseZ);
        g.scale(SortedPiecePointers[i]->size);
        float H = SortedPiecePointers[i]->hue;
        float S = 0.55*!SortedPiecePointers[i]->selected+0.2;
        float V = 1;
        //HSV newColor {static_cast<float>(SortedPiecePointers[i]->hue), 0.75*SortedPiecePointers[i]->selected, 1};
        HSV newColor {H, S, V};
        g.color(newColor);
        Mesh mMesh;
        if (SortedPiecePointers[i]->octaveMod == 1) {
            addSphere(mMesh, 1.0);
        }
        else if (SortedPiecePointers[i]->octaveMod < 1) {
            addCube(mMesh, false, 1.0);
        }
        else if (SortedPiecePointers[i]->octaveMod > 1) {
            addAnnulus(mMesh);
        }
        g.draw(mMesh);
        g.popMatrix();
    }
    for (size_t i = 0; i < Pieces.size(); i++) {
        Pieces[i].updatePiece();
    }

    synthManager.render(g);
    imguiDraw();
  }

  bool onKeyDown(Keyboard const &k) override
  {
    if (ParameterGUI::usingKeyboard())
    {
      return true;
    }

    // std::cout << k.key() << std::endl;

    switch (k.key())
    {

    case '1' : {
        if (1 <= Pieces.size()) {
           Pieces[0].selected = !Pieces[0].selected;
        }
        return false;
    }
    case '2' : {
        if (2 <= Pieces.size()) {
           Pieces[1].selected = !Pieces[1].selected; 
        }
        return false;
    }
    case '3' : {
        if (3 <= Pieces.size()) {
           Pieces[2].selected = !Pieces[2].selected;
        }        return false;
    }
    case '4' : {
        if (4 <= Pieces.size()) {
           Pieces[3].selected = !Pieces[3].selected;
        }        return false;
    }
    case '5' : {
        if (5 <= Pieces.size()) {
           Pieces[4].selected = !Pieces[4].selected;
        }        return false;
    }
    case '6' : {
        if (6 <= Pieces.size()) {
           Pieces[5].selected = !Pieces[5].selected;
        }        return false;
    }
    case '7' : {
        if (7 <= Pieces.size()) {
           Pieces[6].selected = !Pieces[6].selected; 
        }        return false;
    }
    case '8' : {
        for (int i = 0; i < Pieces.size(); i++) {
            Pieces[i].selected = true;
        }
        return false;
    }
    case 96 : {     // ~ key
        for (size_t i = 0; i < Pieces.size(); i++) {
            Pieces[i].selected = false;
        }
        return false;
    }

    case 'z' : {
        modifyPieces(-12, Notes[0]);
        return false;
    }
    case 's' : {
        modifyPieces(-10, Notes[1]);
        return false;
    }
    case 'x' : {
        modifyPieces(-8, Notes[2]);
        return false;
    }
    case 'd' : {
        modifyPieces(-6, Notes[3]);
        return false;
    }
    case 'c' : {
        modifyPieces(-4, Notes[4]);
        return false;
    }
    case 'v' : {
        modifyPieces(-2, Notes[5]);
        return false;
    }
    case 'g' : {
        modifyPieces(0, Notes[6]);
        return false;
    }
    case 'b' : {
        modifyPieces(2, Notes[7]);
        return false;
    }
    case 'h' : {
        modifyPieces(4, Notes[8]);
        return false;
    }
    case 'n' : {
        modifyPieces(6, Notes[9]);
        return false;
    }
    case 'j' : {
        modifyPieces(8, Notes[10]);
        return false;
    }
    case 'm' : {
        modifyPieces(10, Notes[11]);
        return false;
    }
    case ',' : {
        modifyPieces(12, Notes[12]);
        return false;
    }

    case 'w' : {
        if (!crownCreated) {
            crownCreated = true;
            int circleNum = 3;
            for (int i = 0; i < circleNum; i ++) {
                playNote(Fsharp4, 0, 1000, 0.05);
                double x = 3*cos((i*360.0/circleNum)*3.141592/180);
                double z = -50.0 + 3*sin((i*360.0/circleNum)*3.141592/180);
                Piece newPiece {x, z, 0, 0, rateMax, 0, i*1.0/circleNum, 1, false, 1};
                Pieces.push_back(newPiece);
            }
            createSortedPointers();
            return false;
        }
    }
    case 'e' : {
        if (!crownCreated) {
            crownCreated = true;
            int circleNum = 7;
            for (int i = 0; i < circleNum; i ++) {
                playNote(Fsharp4, 0, 1000, 0.05);
                double x = 3*cos((i*360.0/circleNum)*3.141592/180);
                double z = -50.0 + 3*sin((i*360.0/circleNum)*3.141592/180);
                Piece newPiece {x, z, 0, 0, rateMax, 0, i*1.0/circleNum, 1, false, 1};
                Pieces.push_back(newPiece);
            }
            createSortedPointers();
            return false;
        }
    }

    case 0 : {      // shift
        resetPieceSizes();
        return false;
    }
    case 270 : {    // up arrow
        scalePieces(1);
        return false;
    }
    case 272 : {    // down arrow
        scalePieces(-1);
        return false;
    }
    case 269 : {    // left arrow
        shiftPiecesDown();
        return false;
    }
    case 271 : {    // right array
        shiftPiecesUp();
        return false;
    }

    case 91 : {     // {
        modifyOctaves(0.5);
        return false;
    }
    case 93 : {     // }
        modifyOctaves(2);
        return false;
    }
    case 92 : {     // |
        resetOctaves();
        return false;
    }

    case 3 : {      // enter
        autoDeselect = !autoDeselect;
        return false;
    }

    }

    return true;
  }

  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }
  

  void playNote(float freq, float time = 0, float duration = 1, float amp = 0.2, float attack = 0.1, float decay = 0.1)
  {
    SquareWave *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    const float gain = 0.5f;
    voice->setTriggerParams({amp*gain, freq, attack, decay, 0.0, 0.5, time});
    Voices.push_back(voice);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }
};


int main()
{
  MyApp app;

  app.configureAudio(48000., 512, 2, 0);

  app.start();

  return 0;
}

/*

w + e     create 3 or 7 pieces
1-7         selects/deselects the notes
~           deselects all pieces
8           selects all pieces
bottom letters correspond to notes
up and down make pieces larger and smaller
shift       resets all piece sizes
left + right moves selected pieces up and down
{ }         brings select pieces' octaves up and down
|           resets octaves
enter       toggle autodeselect

*/