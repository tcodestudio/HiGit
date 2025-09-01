import { PromptAction } from "@kit.ArkUI"

export class ToastHook {
  private context: UIContext;
  private promptAction: PromptAction;

  constructor(context: UIContext) {
    this.context = context;
    this.promptAction = this.context.getPromptAction();
  }

  showToast(message: string, duration: number = 2000): void {
    this.promptAction.showToast({
      message: message,
      duration: duration
    });
  }

  showError(message: string): void {
    this.promptAction.showToast({
      message: message,
      duration: 3000
    });
  }

  showSuccess(message: string): void {
    this.promptAction.showToast({
      message: message,
      duration: 2000
    });
  }
}