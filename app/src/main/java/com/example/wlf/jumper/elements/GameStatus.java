package com.example.wlf.jumper.elements;

import android.annotation.SuppressLint;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.example.wlf.jumper.devices.DeviceManager;
import com.example.wlf.jumper.devices.LCD;
import com.example.wlf.jumper.devices.LED;
import com.example.wlf.jumper.devices.SevenSegment;

public class GameStatus {
    private int totalScore = 0;
    private int life = 3;
    private int level = 0;
    private int maxLevel = 0;
    private boolean gameClear;


    protected static class Singleton {
        private static final GameStatus instance = new GameStatus();
    }
    public static GameStatus getInstance() { return GameStatus.Singleton.instance; }

    public void init() {
        totalScore = 0;
        life = 3;
        level = 0;
        maxLevel = 0;
        gameClear = false;
        if (DeviceManager.getInstance().isEmbeddedUse()) {
            LED.getInstance().ledInit(this.life);    //life
            SevenSegment.getInstance().SSegmentWrite(0);
        }
    }

    public void sumScore(int score) {
        totalScore += score;
        Log.d("GameStatus", "score : " + this.totalScore);
        if (DeviceManager.getInstance().isEmbeddedUse())
            SevenSegment.getInstance().SSegmentWrite(this.totalScore);
    }

    public int getLife() { return life; }

    public void decreaseLife() {
        life--;
        level = 0;
        Log.d("GameStatus", "life : " + life + ", Max Level : "+ maxLevel);
        if (DeviceManager.getInstance().isEmbeddedUse())
            LED.getInstance().decreaseLife(this.life);   //life
    }
    public void setGameClear() {
        sumScore(life*1200);
        gameClear = true;
    }

    public boolean isGameClear() {
        return gameClear;
    }
    public void printStatus() {
        Log.d("GameStatus", "life : " + life +" score : " + this.totalScore);
    }

    public void increaseLevel() {
        level++;
        if (level > maxLevel) {
            maxLevel = level;
        }
//        DeviceManager.getInstance().LCDWrite();
        LCD.getInstance().lcdWrite();
    }

    public int getLevel() {
        return level;
    }


}
