import { AbilityConstant, ConfigurationConstant, UIAbility, Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { window } from '@kit.ArkUI';
import { WindowUtil } from '../utils/WindowUtil';
import { preferences } from '@kit.ArkData';

const DOMAIN = 0x0000;

export default class EntryAbility extends UIAbility {
  private windowObj?: window.Window;
  private windowUtil?: WindowUtil;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
    this.windowUtil = WindowUtil.getInstance();
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onCreate');
  }

  private onWindowSizeChange: (windowSize: window.Size) => void = (windowSize: window.Size) => {
    this.windowUtil?.updateBreakpoint(windowSize.width);
    AppStorage.setOrCreate('windowWidth', windowSize.width);
  };
  private onWindowStatusChange: (windowStatusType: window.WindowStatusType) => void =
    (windowStatusType: window.WindowStatusType) => {
      this.windowUtil?.updateWindowStatusType(windowStatusType);
    };

  onDestroy(): void {
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // Main window is created, set main page for this ability
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onWindowStageCreate');

    let loadPage = "pages/Index";
    let dataPreferences = preferences.getPreferencesSync(this.context, {
      name: "App"
    });
    if (!dataPreferences.hasSync("agree_app")) {
      loadPage = "pages/Safe";
    }

    windowStage.getMainWindow().then((data: window.Window) => {
      this.windowObj = data;
      this.windowUtil?.updateBreakpoint(this.windowObj.getWindowProperties().windowRect.width);
      AppStorage.setOrCreate('windowWidth', this.windowObj.getWindowProperties().windowRect.width);
      this.windowObj.on('windowSizeChange', this.onWindowSizeChange);
      if (canIUse('SystemCapability.Window.SessionManager')) {
        this.windowObj.on('windowStatusChange', this.onWindowStatusChange);
      }
    })

    windowStage.loadContent(loadPage, (err) => {
      if (err.code) {
        hilog.error(DOMAIN, 'appTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err));
        return;
      }
      hilog.info(DOMAIN, 'appTag', 'Succeeded in loading the content.');
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground(): void {
    // Ability has brought to foreground
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    hilog.info(DOMAIN, 'appTag', '%{public}s', 'Ability onBackground');
  }
}