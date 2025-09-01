export interface NativeResult {
  success: number;
  message: string;
  data: string;
}

export class Result {
  success: boolean;
  message: string;
  data: string;

  constructor(success: number, message: string, data: string) {
    this.success = success == 1;
    this.message = message;
    this.data = data;
  }

  static fromNative(result: NativeResult): Result {
    return new Result(result.success, result.message, result.data);
  }
}