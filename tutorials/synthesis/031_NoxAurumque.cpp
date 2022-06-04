#include <cstdio> // for printing to stdout

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

// using namespace gam;
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
const float Bflat4 = 466.16;
const float B4 = 493.88;

const float C3 = C4/2.0;
const float Csharp3 = Csharp4/2.0;
const float D3 = D4/2.0;
const float Dsharp3 = Dsharp4/2.0;
const float E3 = E4/2.0;
const float F3 = F4/2.0;
const float Fsharp3 = Fsharp4/2.0;
const float G3 = G4/2.0;
const float Gsharp3 = Gsharp4/2.0;
const float A3 = A4/2.0;
const float Bflat3 = Bflat4/2.0;
const float B3 = B4/2.0;

const float C5 = C4*2.0;
const float Csharp5 = Csharp4*2.0;
const float D5 = D4*2.0;
const float Dsharp5 = Dsharp4*2.0;
const float E5 = E4*2.0;
const float F5 = F4*2.0;
const float Fsharp5 = Fsharp4*2.0;
const float G5 = G4*2.0;
const float Gsharp5 = Gsharp4*2.0;
const float A5 = A4*2.0;
const float Bflat5 = Bflat3*2.0;
const float B5 = B4*2.0;

const float C2 = C3/2.0;
const float Csharp2 = Csharp3/2.0;
const float D2 = D3/2.0;
const float Dsharp2 = Dsharp3/2.0;
const float E2 = E3/2.0;
const float F2 = F3/2.0;
const float Fsharp2 = Fsharp3/2.0;
const float G2 = G3/2.0;
const float Gsharp2 = Gsharp3/2.0;
const float A2 = A3/2.0;
const float Bflat2 = Bflat3/2.0;
const float B2 = B3/2.0;

// This example shows how to use SynthVoice and SynthManagerto create an audio
// visual synthesizer. In a class that inherits from SynthVoice you will
// define the synth's voice parameters and the sound and graphic generation
// processes in the onProcess() functions.

struct Ball {
  int r;
  int g;
  int b;
  double x;
  double y;
  double z;
  double radius;
};

std::vector<Ball> Balls {};

class SquareWave : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc7;

  gam::Env<3> mAmpEnv;

  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    createInternalTriggerParameter("amplitude", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
    createInternalTriggerParameter("placement", 1.0, -999.0, 999.0);

    //addDisc(mMesh, 2.0, 30);
    addSphere(mMesh, 1.0);
    //addRect(mMesh, 0.0, 0.0, 1.0, 1.0);
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override
  {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
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
      // float s1 = mAmpEnv() * (mOsc1() * a +
      //                         mOsc3() * (a / 3.0) +
      //                         mOsc5() * (a / 5.0) +
      //                         mOsc7() * (a / 7.0));
      float s1 = mAmpEnv() * (mOsc1() * a +
                              mOsc3() * (a / 3.0) +
                              mOsc5() * (a / 5.0));

      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done())
      free();
  }

    // The graphics processing function
  void onProcess(Graphics& g) override {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    float placement = getInternalParameterValue("placement");
    timer.stop();
    // float secondsModTwelve = 2*timer.elapsedSec();
    // while (secondsModTwelve > 15.0) {
    //   secondsModTwelve -= 15.0;
    // }

    std::cout << "frequency at onProcess: " << frequency << std::endl;
    std::cout << "placement at onProcess: " << placement << std::endl;

    g.pushMatrix();
    // g.translate((rand()%100-50)/40.0, (frequency - C5)/275, -6.8 - timer.elapsedSec()/9.5);
    g.translate(placement, (frequency - C5)/275, -6.8 - timer.elapsedSec()/9.5);
    // g.translate(cos((frequency - F2)/150), sin((frequency - F2)/150), (rand()%100-100)/40.0 - 7);
    // g.translate(cos((frequency - F2)/150), sin((frequency - F2)/150), -12 + secondsModTwelve);
    float randVal = (rand()%100-50)/800.0;
    g.scale(amplitude*1.5 + randVal, amplitude*1.5 + randVal);
    // g.scale(amplitude/2 - (rand()%100-50)/800.0, amplitude/2 + (rand()%100-50)/800.0);
    g.color(-(frequency - C5)/1000 + 0.5 + (rand()%100-50)/300.0 + timer.elapsedSec()/300.0, timer.elapsedSec()/30.0, (frequency - C5)/1000 + 0.5 + (rand()%100-50)/300.0, 0.4);
    g.draw(mMesh);
    g.popMatrix();

    // for (int i = 0; i < 5; i++) {
    //   g.pushMatrix();
    //   g.translate(cos((frequency - F2)/150), sin((frequency - F2)/150), -15 + secondsModTwelve - (rand()%100-50)/15.0);
    //   float randVal = (rand()%100-50)/800.0;
    //   g.scale(amplitude*1.5 + randVal, amplitude*1.5 + randVal);
    //   g.color(-(frequency - C5)/1000 + 0.5 + (rand()%100-50)/300.0 + timer.elapsedSec()/300.0, timer.elapsedSec()/30.0, (frequency - C5)/1000 + 0.5 + (rand()%100-50)/300.0, 0.4);
    //   g.draw(mMesh);
    //   g.popMatrix();
    // }
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App
{
public:
  // GUI manager for SquareWave voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<SquareWave> synthManager{"SquareWave"};

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override
  {
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Play example sequence. Comment this line to start from scratch
    // synthManager.synthSequencer().playSequence("synth1.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
  }

  void onAnimate(double dt) override
  {
    // The GUI is prepared here
    imguiBeginFrame();
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  // The graphics callback function.
  void onDraw(Graphics &g) override
  {
    g.clear();
    // Render the synth's graphics
    synthManager.render(g);

    // GUI is drawn here
    imguiDraw();
  }

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const &k) override
  {
    if (ParameterGUI::usingKeyboard())
    { // Ignore keys if GUI is using
      // keyboard
      return true;
    }

    switch (k.key())
    {

    case 'b':
      std::cout << "b pressed!" << std::endl;
      playSequenceB();
      return false;

    case 'a': {
      std::cout << "a pressed!" << std::endl;
      Ball newBall {255, 50, 50, 1, 1, 1, 3};
      Balls.push_back(newBall);
      std::cout << Balls[0].r << std::endl;
      std::cout << Balls[0].g << std::endl;
      std::cout << Balls[0].b << std::endl;
      std::cout << Balls[0].x << std::endl;
      std::cout << Balls[0].y << std::endl;
      std::cout << Balls[0].z << std::endl;
      std::cout << Balls[0].radius << std::endl;
      return false;

    }

    case 'c':
      std::cout << "c pressed!" << std::endl;
      playSequenceC();
      return false;

    case '1':
      std::cout << "1 pressed!" << std::endl;
      playSequenceB(1.0);
      return false;
    
    case '2':
      std::cout << "2 pressed!" << std::endl;
      playSequenceB(pow(2.0, 2.0/12.0));
      return false;

    case '3':
      std::cout << "3 pressed!" << std::endl;
      playSequenceB(pow(2.0, 3.0/12.0));
      return false;

    case '4':
      std::cout << "4 pressed!" << std::endl;
      playSequenceB(pow(2.0, 4.0/12.0));
      return false;

    case '5':
      std::cout << "5 pressed!" << std::endl;
      playSequenceB(pow(2.0, 5.0/12.0));
      return false;

    case '6':
      std::cout << "6 pressed!" << std::endl;
      playSequenceB(pow(2.0, 6.0/12.0));
      return false;

    case '7':
      std::cout << "7 pressed!" << std::endl;
      playSequenceB(pow(2.0, 7.0/12.0));
      return false;

    case '8':
      std::cout << "8 pressed!" << std::endl;
      playSequenceB(pow(2.0, 8.0/12.0));
      return false;

    case '9':
      std::cout << "9 pressed!" << std::endl;
      playSequenceB(pow(2.0, 9.0/12.0));
      return false;

    }

    return true;
  }

  // Whenever a key is released this function is called
  bool
  onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }

  // New code: a function to play a note A

  void playNote(float freq, float time, float duration = 0.5, float amp = 0.2, float attack = 0.1, float decay = 0.1)
  {
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    const float gain = 0.5f;
    // float newTime = time;
    // while (newTime > 4) {
    //   newTime -= 4;
    // }
    // newTime = newTime/2.0 - 1;
    // std::cout << newTime << std::endl;
    amp /= 2;
    std::cout << "placement at playNote: " << time << std::endl;
    // std::cout << pos() << std::endl;
    voice->setTriggerParams({amp*gain, freq, attack, decay, 0.0, 0.5, time});
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playSequenceB(float offset = 1.0)
  {

    themeA1(1);
    themeA1(9);
    themeA1(17);
    themeA1(25);

    soloA(19);

  }

  void themeA1(int beat = 0) {
    playNote(Csharp5, 2+beat, 5);
    playNote(Dsharp5, 4+beat, 3);

    playNote(E4, 0+beat, 4);
    playNote(Csharp4, 0+beat, 4);
    playNote(B4, 4+beat, 3);
    playNote(Gsharp4, 4+beat, 3);

    playNote(E4, 0+beat, 7);
    playNote(Dsharp4, 4+beat, 3);

    playNote(Csharp4, 0+beat, 7);
    playNote(Gsharp3, 4+beat, 3);
  }

  void themeA2(int beat = 0) {
    playNote(Csharp5, 2+beat, 2.01, 0.2, 2, 3);
    playNote(Dsharp5, 4+beat, 0.01, 0.2, 0, 3);

    playNote(E4, 0+beat, 4, 0.2, 4, 0);
    playNote(Csharp4, 0+beat, 4, 0.2, 4, 0);
    playNote(B4, 4+beat, 0.01, 0.2, 0, 3);
    playNote(Gsharp4, 4+beat, 0.01, 0.2, 0, 3);

    playNote(E4, 0+beat, 4.01, 0.2, 4, 3);
    playNote(Dsharp4, 4+beat, 0.01, 0.2, 0, 3);

    playNote(Csharp4, 0+beat, 4.01, 0.2, 4, 3);
    playNote(Gsharp3, 4+beat, 0.01, 0.2, 0, 3);
  }

  void soloA(int beat = 0) {
    playNote(Csharp5, beat, 2, 0.25);
    playNote(Dsharp5, 2+beat, 2, 0.25);
    playNote(Gsharp5, 4+beat, 5, 0.3);
    playNote(Csharp5, 9+beat, 1, 0.25);
    playNote(Dsharp5, 10+beat, 3, 0.25);
  }

  void playSequenceC() {
    themeB();
  }

  void themeB() {
    timer.start();
    playNote(A5, 2.666, 0.666, 0.6);
    playNote(B5, 3.333, 0.666, 0.6);
    playNote(C5, 4, 2, 0.4);
    playNote(A5, 6.666, 0.666, 0.6);
    playNote(B5, 7.333, 0.666, 0.6);
    playNote(C5, 8, 2, 0.4);
    playNote(A5, 10.666, 0.666, 0.6);
    playNote(G5, 11.333, 0.666, 0.6);
    playNote(B4, 12, 0.5, 0.5);
    playNote(A4, 12.5, 0.5, 0.5);
    playNote(B4, 13, 5, 0.4);
    playNote(A5, 18.666, 0.666, 0.6);
    playNote(B5, 19.333, 0.666, 0.6);
    playNote(C5, 20, 2, 0.4);
    playNote(A5, 22.666, 0.666, 0.6);
    playNote(B5, 23.333, 0.666, 0.6);
    playNote(C5, 24, 2, 0.4);
    playNote(A5, 26.666, 0.666, 0.65);
    playNote(G5, 27.333, 0.666, 0.65);
    playNote(B5, 28, 4, 0.7);
    playNote(A5, 32, 2, 0.7);
    playNote(E5, 35, 1, 0.7);
    playNote(G5, 36, 3, 0.7);
    playNote(C5, 39, 1, 0.7);
    playNote(C5, 40, 1, 0.7);
    playNote(D5, 41, 1, 0.7);
    playNote(E5, 43, 1, 0.45);
    playNote(G5, 44, 3, 0.45);
    playNote(C5, 47, 1, 0.45);
    playNote(D5, 48, 1, 0.45);
    playNote(C5, 48, 1, 0.45);
    playNote(E4, 51, 1, 0.3);
    playNote(G4, 52, 3, 0.3);
    playNote(F4, 52, 3, 0.3);
    playNote(C4, 55, 1, 0.3);
    playNote(D4, 56, 3, 0.3);

    playNote(D5, 3, 1, 0.5);
    playNote(C4, 4, 0.5, 0.6);
    playNote(B4, 4.5, 0.5, 0.6);
    playNote(A4, 5, 1, 0.6);
    playNote(E4, 6, 2, 0.4);
    playNote(C4, 8, 0.5, 0.6);
    playNote(B4, 8.5, 0.5, 0.6);
    playNote(G4, 9, 1, 0.6);
    playNote(E4, 10, 2, 0.4);
    playNote(E4, 12, 0.666, 0.5);
    playNote(E4, 12.666, 0.666, 0.5);
    playNote(E4, 13.333, 0.666, 0.5);
    playNote(A4, 14, 0.5, 0.5);
    playNote(B4, 14.5, 0.5, 0.5);
    playNote(G4, 16.5, 0.5, 0.6);
    playNote(G4, 17, 0.5, 0.6);
    playNote(G5, 17.5, 0.5, 0.6);
    playNote(Fsharp5, 18, 1, 0.6);
    playNote(D5, 19, 1, 0.6);
    playNote(E5, 20, 0.5, 0.6);
    playNote(D5, 20.5, 0.5, 0.6);
    playNote(C5, 21, 1, 0.4);
    playNote(G5, 22, 2, 0.5);
    playNote(E5, 24, 0.5, 0.65);
    playNote(D5, 24.5, 0.5, 0.65);
    playNote(C5, 25, 1, 0.4);
    playNote(G5, 26, 2, 0.5);
    playNote(G5, 28, 1, 0.7);
    playNote(A5, 29, 2, 0.7);
    playNote(G5, 31, 1, 0.7);
    playNote(F5, 32, 2, 0.7);
    playNote(E5, 35, 1, 0.7);
    playNote(F5, 36, 3, 0.7);
    playNote(E5, 36, 3, 0.7);
    playNote(C5, 39, 1, 0.7);
    playNote(B4, 40, 1, 0.7);
    playNote(B4, 41, 1, 0.7);
    playNote(E5, 43, 1, 0.45);
    playNote(F5, 44, 3, 0.45);
    playNote(E5, 44, 3, 0.45);
    playNote(C5, 47, 1, 0.45);

    playNote(C4, 5, 1, 0.4);
    playNote(G4, 6, 1, 0.5);
    playNote(C5, 7, 1, 0.6);
    playNote(C4, 9, 1, 0.4);
    playNote(G4, 10, 1, 0.5);
    playNote(C5, 11, 1, 0.6);
    playNote(G4, 13, 0.5, 0.4);
    playNote(G4, 13.5, 0.5, 0.4);
    playNote(D5, 14, 0.666, 0.6);
    playNote(D5, 14.666, 0.666, 0.6);
    playNote(D5, 15.333, 0.333, 0.6);
    playNote(G4, 15.666, 0.333, 0.6);
    playNote(A4, 16, 4, 0.4);
    playNote(A4, 20.666, 0.666, 0.6);
    playNote(B4, 21.333, 0.666), 0.6;
    playNote(C4, 22, 1, 0.5);
    playNote(E4, 23, 1, 0.5);
    playNote(A4, 24.666, 0.666, 0.6);
    playNote(D4, 25.333, 0.666, 0.6);
    playNote(E4, 26, 1, 0.6);
    playNote(C5, 27, 1, 0.6);
    playNote(D5, 28, 8, 0.7);
    playNote(B4, 28, 4, 0.7);
    playNote(A4, 32, 4, 0.7);
    playNote(C5, 36, 3, 0.7);
    playNote(A4, 36, 3, 0.7);
    playNote(A4, 39, 1, 0.7);
    playNote(G4, 40, 1, 0.7);
    playNote(G4, 41, 3, 0.7);
    playNote(C5, 44, 3, 0.45);
    playNote(A4, 44, 2, 0.45);
    playNote(G4, 46, 1, 0.45);
    playNote(F4, 47, 1, 0.45);
    playNote(B4, 48, 1, 0.45);
    playNote(G4, 48, 1, 0.45);
    playNote(E4, 51, 1, 0.3);
    playNote(E4, 52, 3, 0.3);
    playNote(C4, 52, 3, 0.3);
    playNote(C4, 55, 1, 0.3);
    playNote(C4, 56, 3, 0.3);

    playNote(G3, 2, 1, 0.4);
    playNote(G4, 3, 1, 0.4);
    playNote(E4, 4, 1, 0.5);
    playNote(E4, 5, 0.5, 0.6);
    playNote(D4, 5.5, 0.5, 0.6);
    playNote(C4, 6, 0.5, 0.6);
    playNote(D4, 6.5, 0.5, 0.6);
    playNote(E4, 7, 1, 0.5);
    playNote(E4, 8, 1, 0.5);
    playNote(E4, 9, 0.5, 0.6);
    playNote(D4, 9.5, 0.5, 0.6);
    playNote(C4, 10, 0.5, 0.6);
    playNote(D4, 10.5, 0.5, 0.6);
    playNote(E4, 11, 1, 0.5);
    playNote(B3, 13, 1, 0.45);
    playNote(G4, 14, 1, 0.6);
    playNote(Fsharp4, 15, 1, 0.5);
    playNote(D4, 16, 2, 0.4);
    playNote(D4, 18, 0.5, 0.6);
    playNote(D4, 18.5, 0.5, 0.6);
    playNote(Fsharp4, 19, 0.5, 0.6);
    playNote(G4, 19.5, 0.5, 0.6);
    playNote(C4, 21, 1, 0.4);
    playNote(E4, 22, 1, 0.5);
    playNote(G4, 23, 1, 0.6);
    playNote(C4, 25, 1, 0.4);
    playNote(G4, 26, 1, 0.5);
    playNote(E4, 27, 1, 0.6);
    playNote(D4, 28, 2, 0.7);
    playNote(B3, 28, 2, 0.7);
    playNote(A4, 30, 1, 0.7);
    playNote(D4, 30, 1, 0.7);
    playNote(G4, 31, 1, 0.7);
    playNote(D4, 31, 1, 0.7);
    playNote(F4, 32, 4, 0.7);
    playNote(D4, 32, 4, 0.7);
    playNote(A3, 36, 2, 0.7);
    playNote(F4, 38, 1, 0.7);
    playNote(G4, 39, 1, 0.7);
    playNote(C4, 38, 2, 0.7);
    playNote(D4, 40, 1, 0.7);
    playNote(B3, 40, 1, 0.7);
    playNote(D4, 41, 1, 0.7);
    playNote(B3, 41, 1, 0.7);
    playNote(A3, 44, 1, 0.45);
    playNote(G4, 45, 1, 0.45);
    playNote(F4, 46, 1, 0.45);
    playNote(C4, 45, 2, 0.45);
    playNote(E4, 47, 1, 0.45);
    playNote(C4, 47, 1, 0.45);
    playNote(D4, 48, 1, 0.45);
    playNote(A3, 52, 2, 0.3);
    playNote(G3, 52, 2, 0.3);
    playNote(A3, 54, 2, 0.3);
    playNote(G3, 54, 2, 0.3);
    playNote(B3, 56, 3, 0.3);

    playNote(D4, 3, 1, 0.55);
    playNote(G3, 3, 1, 0.55);
    playNote(E3, 4, 3, 0.55);
    playNote(A2, 4, 3, 0.55);
    playNote(A3, 7, 1, 0.55);
    playNote(E3, 7, 1, 0.55);
    playNote(C3, 8, 4, 0.55);
    playNote(G3, 8, 4, 0.55);
    playNote(E3, 12, 2, 0.55);
    playNote(B3, 14, 2, 0.55);
    playNote(E3, 14, 2, 0.55);
    playNote(B3, 16, 0.5, 0.7);
    playNote(D3, 16, 0.5, 0.7);
    playNote(B3, 16.5, 0.5, 0.7);
    playNote(D3, 16.5, 0.5, 0.7);
    playNote(B3, 17, 2, 0.6);
    playNote(D3, 17, 2, 0.6);
    playNote(B3, 19, 1, 0.55);
    playNote(A3, 19, 1, 0.55);
    playNote(E3, 20, 3, 0.55);
    playNote(A2, 20, 3, 0.55);
    playNote(A3, 23, 1, 0.55);
    playNote(E3, 23, 1, 0.55);
    playNote(C3, 24, 4, 0.6);
    playNote(G3, 24, 4, 0.6);
    playNote(G2, 28, 2, 0.7);
    playNote(D3, 28, 2, 0.7);
    playNote(B3, 30, 1, 0.7);
    playNote(G3, 30, 1, 0.7);
    playNote(B3, 31, 1, 0.7);
    playNote(G3, 31, 1, 0.7);
    playNote(A3, 32, 4, 0.7);
    playNote(F3, 32, 4, 0.7);
    playNote(C3, 36, 2, 0.7);
    playNote(F2, 36, 2, 0.7);
    playNote(A3, 38, 2, 0.7);
    playNote(F3, 38, 2, 0.7);
    playNote(B3, 40, 1, 0.7);
    playNote(G3, 40, 1, 0.7);
    playNote(B3, 41, 1, 0.7);
    playNote(G3, 41, 1, 0.7);
    playNote(C3, 44, 1, 0.45);
    playNote(F2, 44, 1, 0.45);
    playNote(A3, 45, 1, 0.45);
    playNote(F3, 45, 1, 0.45);
    playNote(A3, 46, 2, 0.45);
    playNote(F3, 46, 2, 0.45);
    playNote(G3, 48, 4, 0.45);
    playNote(C3, 52, 4, 0.3);
    playNote(F2, 52, 4, 0.3);
    playNote(D3, 56, 1, 0.3);
    playNote(G2, 56, 1, 0.3);
    playNote(G3, 57, 1, 0.3);
  }
};


int main()
{
  // Create app instance
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();

  return 0;
}