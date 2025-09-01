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

import { PiPWindow } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { CommonConstants } from '../constants/CommonConstants';
import { AvPlayerUtil } from './AvPlayerUtil';
import Logger from './Logger';

export class PipWindowUtil {
  private isShowingPip: boolean = false;
  private pipController?: PiPWindow.PiPController = undefined;
  private mXComponentController: XComponentController = new XComponentController();
  private avPlayerUtil: AvPlayerUtil | undefined = undefined;

  constructor(uiContext: UIContext) {
    this.avPlayerUtil = AvPlayerUtil.getAvPlayerUtil(uiContext);
  }

  static getPipWindowUtil(uiContext: UIContext): PipWindowUtil | undefined {
    if (!AppStorage.get<PipWindowUtil>(CommonConstants.PIP_WINDOW_UTIL)) {
      AppStorage.setOrCreate(CommonConstants.PIP_WINDOW_UTIL, new PipWindowUtil(uiContext));
    }
    return AppStorage.get<PipWindowUtil>(CommonConstants.PIP_WINDOW_UTIL);
  }

  getIsShowingPip(): boolean {
    return this.isShowingPip;
  }

  getPipController(): PiPWindow.PiPController | undefined {
    return this.pipController;
  }

  getXComponentController(): XComponentController {
    return this.mXComponentController;
  }
  // [Start start_pip]
  async startPip(navId: string, mXComponentController: XComponentController, context: Context, pageInfos: NavPathStack): Promise<void> {
    if (canIUse('SystemCapability.Window.SessionManager')) {
      if (!PiPWindow.isPiPEnabled()) {
        Logger.error(`picture in picture disabled for current OS`);
        return;
      }
      let config: PiPWindow.PiPConfiguration = {
        context: context,
        componentController: mXComponentController,
        // Navigation ID of the current page.
        navigationId: navId,
        templateType: PiPWindow.PiPTemplateType.VIDEO_LIVE
      };
      // Create a pip controller.
      let promise: Promise<PiPWindow.PiPController> = PiPWindow.create(config);
      await promise.then((controller: PiPWindow.PiPController) => {
        this.pipController = controller;
        // Initializing the pip controller.
        this.initPipController();
        // Enabling the pip function through the startPip interface.
        this.pipController.startPiP().then(() => {
          Logger.info(`Succeeded in starting pip.`);
          if (this.avPlayerUtil === undefined) {
            return;
          }
          this.avPlayerUtil.play();
          pageInfos.pop();
        }).catch((err: BusinessError) => {
          Logger.error(`Failed to start pip. Cause: ${err.code}, message: ${err.message}`);
        });
      }).catch((err: BusinessError) => {
        Logger.error(`Failed to create pip controller. Cause: ${err.code}, message: ${err.message}`);
      });
    }
  }
  // [End start_pip]
  // [Start init_pip_controller]
  initPipController(): void {
    if (!this.pipController) {
      return;
    }
    if (canIUse('SystemCapability.Window.SessionManager')) {
      this.pipController.on('stateChange', (state: PiPWindow.PiPState, reason: string) => {
        this.onStateChange(state, reason);
      });
      this.pipController.on('controlPanelActionEvent', (event: PiPWindow.PiPActionEventType) => {
        this.onActionEvent(event);
      });
    }
  }
  // [End init_pip_controller]
  onStateChange(state: PiPWindow.PiPState, reason: string): void {
    if (canIUse('SystemCapability.Window.SessionManager')) {
      let curState: string = '';
      switch (state) {
        case PiPWindow.PiPState.ABOUT_TO_START:
          curState = 'ABOUT_TO_START';
          break;
        case PiPWindow.PiPState.STARTED:
          curState = 'STARTED';
          this.isShowingPip = true;
          break;
        case PiPWindow.PiPState.ABOUT_TO_STOP:
          curState = 'ABOUT_TO_STOP';
          break;
        case PiPWindow.PiPState.STOPPED:
          curState = 'STOPPED';
          this.isShowingPip = false;
          break;
        case PiPWindow.PiPState.ABOUT_TO_RESTORE:
          curState = 'ABOUT_TO_RESTORE';
          this.isShowingPip = false;
          break;
        case PiPWindow.PiPState.ERROR:
          curState = 'ERROR';
          this.isShowingPip = false;
          break;
        default:
          break;
      }
      Logger.info(`onStateChange: ${curState}, reason: ${reason}`);
    }
  }

  onActionEvent(event: PiPWindow.PiPActionEventType) {
    switch (event) {
      case 'playbackStateChanged':
        if (this.avPlayerUtil === undefined) {
          return;
        }
        this.avPlayerUtil.playerStateControl();
        break;
      default:
        break;
    }
  }
  // [Start stop_pip]
  // Disable the pip function by calling stopPip.
  async stopPip(): Promise<void> {
    if (canIUse('SystemCapability.Window.SessionManager')) {
      if (this.pipController) {
        let promise: Promise<void> = this.pipController.stopPiP();
        promise.then(() => {
          this.isShowingPip = false;
          Logger.info(`Succeeded in stopping pip.`);
          try {
            this.pipController?.off('stateChange');
            this.pipController?.off('controlPanelActionEvent');
          } catch (exception) {
            Logger.error('Failed to unregister callbacks. Code: ' + JSON.stringify(exception));
          }
        }).catch((err: BusinessError) => {
          Logger.error(`Failed to stop pip. Cause: ${err.code}, message: ${err.message}`);
        });
      }
    }
  }
  // [End stop_pip]
}