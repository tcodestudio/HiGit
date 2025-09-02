import { ToastHook } from '../utils/ToastHook';

export default class BaseViewModel {
  protected toastHook?: ToastHook;

  setToastHook(hook: ToastHook) {
    this.toastHook = hook;
  }
}