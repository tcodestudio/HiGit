/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { media } from '@kit.MediaKit';
import { resourceManager } from '@kit.LocalizationKit';
import { BusinessError } from '@kit.BasicServicesKit';
import Logger from './Logger';
import { CommonConstants } from '../constants/CommonConstants';

export class AvPlayerUtil {
  private avPlayer?: media.AVPlayer;
  private url: resourceManager.RawFileDescriptor | null = null;
  private playState: boolean = true;
  private surfaceId: string = '';
  private onError: (err: BusinessError) => void = (err: BusinessError) => {
    Logger.error(`Invoke avPlayer failed, code is ${err.code}, message is ${err.message}`);
    if (this.avPlayer === undefined) {
      Logger.error(`AvPlayer is undefined`);
      return;
    }
    this.avPlayer.reset();
  }
  private onStateChange: (state: media.AVPlayerState) => void
  private uiContext: UIContext | undefined = undefined;

  constructor(uiContext: UIContext) {
    this.uiContext = uiContext;
    this.onStateChange = async (state: media.AVPlayerState) => {
      if (this.avPlayer === undefined) {
        Logger.error(`AvPlayer is undefined`);
        return;
      }
      switch (state) {
        case CommonConstants.AV_PLAYER_IDLE_STATE:
          this.url = await uiContext!.getHostContext()!.resourceManager.getRawFd(CommonConstants.LIVE_VIDEO_NAME);
          this.avPlayer.fdSrc = this.url;
          Logger.info('AVPlayer state idle called.');
          break;
        case CommonConstants.AV_PLAYER_INITIALIZED_STATE:
          Logger.info('AVPlayer initialized called.');
          this.avPlayer.surfaceId = this.surfaceId;
          this.avPlayer.prepare().then(() => {
            Logger.info('AVPlayer prepare succeeded.');
          }, (err: BusinessError) => {
            Logger.error(`Invoke prepare failed, code is ${err.code}, message is ${err.message}`);
            if (this.avPlayer === undefined) {
              Logger.error(`AvPlayer is undefined`);
              return;
            }
            this.avPlayer.reset();
          });
          break;
        case CommonConstants.AV_PLAYER_PREPARED_STATE:
          this.avPlayer.videoScaleType = media.VideoScaleType.VIDEO_SCALE_TYPE_FIT;
          Logger.info('AVPlayer state prepared called.');
          this.avPlayer.play();
          break;
        case CommonConstants.AV_PLAYER_PLAYING_STATE:
          Logger.info('AVPlayer state playing called.');
          this.playState = true;
          break;
        case CommonConstants.AV_PLAYER_PAUSED_STATE:
          Logger.info('AVPlayer state paused called.');
          this.playState = false;
          break;
        case CommonConstants.AV_PLAYER_COMPLETED_STATE:
          Logger.info('AVPlayer state completed called.');
          this.playState = false;
          this.avPlayer.stop();
          break;
        case CommonConstants.AV_PLAYER_STOPPED_STATE:
          Logger.info('AVPlayer state stopped called.');
          break;
        case CommonConstants.AV_PLAYER_RELEASE_STATE:
          Logger.info('AVPlayer state released called.');
          break;
        case CommonConstants.AV_PLAYER_ERROR_STATE:
          Logger.error('AVPlayer state error called.');
          break;
        default:
          Logger.info('AVPlayer state unknown called.');
          break;
      }
    }
  }

  static getAvPlayerUtil(uiContext: UIContext): AvPlayerUtil | undefined {
    if (!AppStorage.get<AvPlayerUtil>(CommonConstants.AV_PLAYER_UTIL)) {
      AppStorage.setOrCreate(CommonConstants.AV_PLAYER_UTIL, new AvPlayerUtil(uiContext));
    }
    return AppStorage.get<AvPlayerUtil>(CommonConstants.AV_PLAYER_UTIL);
  }

  async createAvPlayer(surfaceId: string): Promise<void> {
    if (this.avPlayer === undefined || this.avPlayer.state === CommonConstants.AV_PLAYER_RELEASE_STATE) {
      this.avPlayer = await media.createAVPlayer();
      this.surfaceId = surfaceId;
      Logger.info('Created AvPlayer successfully.');
      // Entering idle state.
      this.url = await this.uiContext!.getHostContext()!.resourceManager.getRawFd(CommonConstants.LIVE_VIDEO_NAME);
      this.avPlayer.fdSrc = this.url;
      this.setAVPlayerCallback();
    } else {
      Logger.info(`AvPlayer has been created`);
    }
  }

  setAVPlayerCallback(): void {
    if (this.avPlayer === undefined) {
      Logger.error(`AvPlayer is undefined`);
      return;
    }
    this.avPlayer.on('error', this.onError);
    this.setStateChange();
  }

  setStateChange(): void {
    if (this.avPlayer === undefined) {
      Logger.error(`AvPlayer is undefined`);
      return;
    }
    this.avPlayer.on('stateChange', this.onStateChange)
  }

  release(): void {
    if (this.avPlayer !== undefined && this.avPlayer.state !== CommonConstants.AV_PLAYER_RELEASE_STATE) {
      try {
        this.avPlayer.off('error');
        this.avPlayer.off('stateChange');
      } catch (exception) {
        Logger.error('Failed to unregister the error and state callback. Code: ' + JSON.stringify(exception));
      }
      this.avPlayer.release();
    } else {
      Logger.info(`AvPlayer release failed`);
    }
  }

  playerStateControl(): void {
    if (this.avPlayer === undefined) {
      Logger.error(`AvPlayer is undefined`);
      return;
    }
    if (this.avPlayer.state === CommonConstants.AV_PLAYER_STOPPED_STATE) {
      this.avPlayer.prepare();
      return;
    }
    if (!this.playState) {
      this.avPlayer.play();
    } else {
      this.avPlayer.pause();
    }
  }

  play(): void {
    if (this.avPlayer !== undefined && !this.playState) {
      this.avPlayer.play();
    } else {
      Logger.info(`AvPlayer play failed`);
    }
  }

  pause(): void {
    if (this.avPlayer !== undefined && this.playState) {
      this.avPlayer.pause();
    } else {
      Logger.info(`AvPlayer pause failed`);
    }
  }
}