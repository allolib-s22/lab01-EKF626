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

const float gravity = 0.01;
const double rateMax = 0.2;
int selectedNote = 1;

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

struct Piece {
    double baseX;
    double baseY;
    double baseZ;
    double currentY;
    double targetY;
    double yRate;
    int reachedNewTarget;
    double h;
    void updatePiece();
    void setNewTarget(double target);
};

std::vector<Piece> Pieces {};

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

class MyApp : public App
{
public:
  SynthGUIManager<SquareWave> synthManager{"SquareWave"};

  Mesh mMesh;

  void onCreate() override
  {
    navControl().active(false); 

    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    synthManager.synthRecorder().verbose(true);

    addSphere(mMesh, 1.0);
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

    for (size_t i = 0; i < Pieces.size(); i++) {
        g.pushMatrix();
        g.translate(Pieces[i].baseX, Pieces[i].currentY, Pieces[i].baseZ);
        g.scale(1);
        //g.color(1, 1, 1);
        HSV newColor {static_cast<float>(Pieces[i].h), 1, 1};
        g.color(newColor);
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

    switch (k.key())
    {

    case '1' : {
        selectedNote = 1;
        return false;
    }
    case '2' : {
        selectedNote = 2;
        return false;
    }
    case '3' : {
        selectedNote = 3;
        return false;
    }
    case '4' : {
        selectedNote = 4;
        return false;
    }
    case '5' : {
        selectedNote = 5;
        return false;
    }
    case '6' : {
        selectedNote = 6;
        return false;
    }
    case '7' : {
        selectedNote = 7;
        return false;
    }

    case 'z' : {
        Pieces[selectedNote-1].setNewTarget(-12);
        return false;
    }
    case 's' : {
        Pieces[selectedNote-1].setNewTarget(-10);
        return false;
    }
    case 'x' : {
        Pieces[selectedNote-1].setNewTarget(-8);
        return false;
    }
    case 'd' : {
        Pieces[selectedNote-1].setNewTarget(-6);
        return false;
    }
    case 'c' : {
        Pieces[selectedNote-1].setNewTarget(-4);
        return false;
    }
    case 'v' : {
        Pieces[selectedNote-1].setNewTarget(-2);
        return false;
    }
    case 'g' : {
        Pieces[selectedNote-1].setNewTarget(0);
        return false;
    }
    case 'b' : {
        Pieces[selectedNote-1].setNewTarget(2);
        return false;
    }
    case 'h' : {
        Pieces[selectedNote-1].setNewTarget(4);
        return false;
    }
    case 'n' : {
        Pieces[selectedNote-1].setNewTarget(6);
        return false;
    }
    case 'j' : {
        Pieces[selectedNote-1].setNewTarget(8);
        return false;
    }
    case 'm' : {
        Pieces[selectedNote-1].setNewTarget(10);
        return false;
    }
    case ',' : {
        Pieces[selectedNote-1].setNewTarget(12);
        return false;
    }

    case 'q' : {
        playNote(261.62, 0, 4, 0.3);
        return false;
    }

    case 'w' : {
        Piece newPiece {0, 0, -50, 0, 0, rateMax, 0, 0.5};
        Pieces.push_back(newPiece);
        return false;
    }

    case 'e' : {
        int circleNum = 7;
        for (int i = 0; i < circleNum; i ++) {
            // Piece newPiece {-10.0 + 3*i, 0, -40.0 + 3*i, 0, 0, rateMax, 0, i*0.1};
            double x = 5*cos((i*360.0/circleNum)*3.141592/180);
            double z = -50.0 + 5*sin((i*360.0/circleNum)*3.141592/180);
            Piece newPiece {x, 0, z, 0, 0, rateMax, 0, i*1.0/circleNum};
            Pieces.push_back(newPiece);
        }
        return false;
    }

    case 'r' : {
        int randNum = rand()%Pieces.size();
        Pieces[randNum].setNewTarget(5);
        return false;
    }

    case 't' : {
        int randNum = rand()%Pieces.size();
        Pieces[randNum].setNewTarget(-5);
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
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    const float gain = 0.5f;
    voice->setTriggerParams({amp*gain, freq, attack, decay, 0.0, 0.5, time});
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