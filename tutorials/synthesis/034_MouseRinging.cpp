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

int x;
int y;
int dx;
int dy;

double randNum;


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
    //float f = (x+y)/2;
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

    if (timer.elapsedSec() > 0.1) {
        randNum = x + rand()%(y)/5.0;
        timer.start();
        playNote(randNum);
    }
    g.pushMatrix();
    g.translate(0, 0, -20);
    g.scale(randNum/500);
    HSV newColor {static_cast<float>(randNum)/1000, 1, 1};
    g.color(newColor);
    g.draw(mMesh);
    g.popMatrix();

    synthManager.render(g);
    imguiDraw();
  }

  bool onMouseMove(Mouse const &m) override {
      //std::cout << "(" << m.x() << ", " << m.y() << ") | (" << m.dx() << ", " << m.dy() << ")" << std::endl;
      x = m.x();
      y = m.y();
      dx = m.dx();
      dy = m.dy();
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
        playNote(261.62, 0, 20, 0.3);
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