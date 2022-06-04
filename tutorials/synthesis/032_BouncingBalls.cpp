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

struct Ball {
  double h;
  double s;
  double v;
  double x;
  double y;
  double z;
  double radius;
  double yRate;
};

std::vector<Ball> Balls {};

void updateBalls() {
    for (size_t i = 0; i < Balls.size(); i++) {
        Balls[i].yRate -= 0.01;
        Balls[i].y += Balls[i].yRate;
        Balls[i].z -= 0.2;
        if (Balls[i].y < -25) {
            Balls.erase(Balls.begin() + i);
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

  Mesh mMesh;


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

    //addDisc(mMesh, 2.0, 30);
    addSphere(mMesh, 1.0);
    //addRect(mMesh, 0.0, 0.0, 1.0, 1.0);
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
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    timer.stop();

    for (size_t i = 0; i < Balls.size(); i++) {
        g.pushMatrix();
        g.translate(Balls[i].x, Balls[i].y, Balls[i].z);
        g.scale(Balls[i].radius);
        // g.color(Balls[i].r/255.0, Balls[i].g/255.0, Balls[i].b/255.0);
        HSV newColor {static_cast<float>(Balls[i].h), static_cast<float>(Balls[i].s), static_cast<float>(Balls[i].v)};
        g.color(newColor);
        g.draw(mMesh);
        g.popMatrix();
    }

  }

  void onTriggerOn() override { 
    mAmpEnv.reset(); 
    std::cout << "TRIGGER ON!!!" << std::endl;
    float f = getInternalParameterValue("frequency");
    std::cout << f << std::endl;
    }

  void onTriggerOff() override { 
    std::cout << "trigger off." << std::endl;
    float f = getInternalParameterValue("frequency");
    std::cout << f << std::endl;
    mAmpEnv.release(); 
    }
};

class MyApp : public App
{
public:
  SynthGUIManager<SquareWave> synthManager{"SquareWave"};

  void onCreate() override
  {
    navControl().active(false); 

    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    synthManager.synthRecorder().verbose(true);
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
    updateBalls();

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

    case '1': {
        playNote(C4, 0, 100, 0.01);
        return false;
    }

    case 'a': {
      std::cout << "a pressed!" << std::endl;
      Ball newBall1 {255, 50, 50, -10, 0, -30, 1, 0.3};
      Balls.push_back(newBall1);
      Ball newBall2 {50, 255, 50, 0, 0, -30, 1, 0.3};
      Balls.push_back(newBall2);
      Ball newBall3 {50, 50, 255, 10, 0, -30, 1, 0.3};
      Balls.push_back(newBall3);
      return false;
    }

    case 'z': {
        std::cout << "z pressed" << std::endl;
        playNote(C4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, -7, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'x': {
        std::cout << "x pressed" << std::endl;
        playNote(D4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, -5, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'c': {
        std::cout << "c pressed" << std::endl;
        playNote(E4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, -3, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'v': {
        std::cout << "v pressed" << std::endl;
        playNote(F4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, -1, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'b': {
        std::cout << "b pressed" << std::endl;
        playNote(G4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, 1, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'n': {
        std::cout << "n pressed" << std::endl;
        playNote(A4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, 3, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case 'm': {
        std::cout << "m pressed" << std::endl;
        playNote(B4, 0);
        Ball newBall {rand()%100/100.0, 1, 1, 5, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
        return false;
    }

    case ',': {
        std::cout << ", pressed" << std::endl;
        playNote(C5, 0);
        Ball newBall {rand()%100/100.0, 1, 1, 7, -10, -30, 1, 0.5};
        Balls.push_back(newBall);
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

  void playNote(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1)
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