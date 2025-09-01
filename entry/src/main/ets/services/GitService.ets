import nativeApi from 'libentry.so';
import { Result } from '../data/Result';
import { emitter } from '@kit.BasicServicesKit';
import { CommitItem } from '../data/Commit'

export function parseCommitList(data: string): Array<CommitItem> {
  return JSON.parse(data) as Array<CommitItem>;
}

@Concurrent
export async function initGit(path: string): Promise<void> {
  nativeApi.initSystem(path);
}

@Concurrent
export async function getSSHKey(): Promise<string> {
  const result = nativeApi.getSSHKey();
  return result.data;
}

@Concurrent
export async function generateSSHKey(): Promise<Result> {
  const result = nativeApi.generateSSHKey();
  return Result.fromNative(result);
}

@Concurrent
export async function initRepo(path: string, url: string, repo: string, provider: string): Promise<Result> {
  const result = nativeApi.initRepo(path, url, repo, provider);
  return Result.fromNative(result);
}

@Concurrent
export async function deleteRepo(path: string, url: string, repo: string, provider: string): Promise<Result> {
  const result = nativeApi.deleteRepo(path, url, repo, provider);
  return Result.fromNative(result);
}

@Concurrent
export async function getBranches(url: string): Promise<Result> {
  const result = nativeApi.getBranches(url);
  return Result.fromNative(result);
}

@Concurrent
export async function getTags(url: string): Promise<Result> {
  const result = nativeApi.getTags(url);
  return Result.fromNative(result);
}

@Concurrent
export async function getCommits(url: string, branch: string, count: number, offset: number): Promise<Result> {
  const result = nativeApi.history(url, branch, count, offset);
  return Result.fromNative(result);
}

@Concurrent
export async function fetchBranch(url: string, branch: string): Promise<Result> {
  const maxRetries = 3;
  let lastResult: Result | null = null;

  for (let attempt = 1; attempt <= maxRetries; attempt++) {
    try {
      const nativeResult = nativeApi.fetch(url, branch, (process, total, message) => {
        emitter.emit("fetch",
          { data: { "process": process.toString(), "total": total.toString(), "message": message } })
      });
      const result = Result.fromNative(nativeResult);
      // 检查是否成功
      if (result.success) {
        return result;
      }

      // 保存失败的结果
      lastResult = result;

      // 如果不是最后一次尝试，则等待一段时间后重试
      if (attempt < maxRetries) {
        // 等待时间随重试次数递增：100ms, 200ms, 400ms
        const delay = 100 * Math.pow(2, attempt - 1);
        await new Promise<void>(resolve => setTimeout(resolve, delay));
      }
    } catch (error) {
      // 如果发生异常，也保存错误信息
      lastResult = new Result(0, `Attempt ${attempt} failed: ${error}`, '');

      // 如果不是最后一次尝试，则等待一段时间后重试
      if (attempt < maxRetries) {
        // 等待时间随重试次数递增：100ms, 200ms, 400ms
        const delay = 100 * Math.pow(2, attempt - 1);
        await new Promise<void>(resolve => setTimeout(resolve, delay));
      }
    }
  }

  // 所有重试都失败了，返回最后一个失败的结果
  return lastResult!;
}

@Concurrent
export async function getFileTree(url: string, branch: string): Promise<Result> {
  const result = nativeApi.getFileTree(url, branch);
  return Result.fromNative(result);
}

@Concurrent
export async function readFile(url: string, branch: string, path: string): Promise<Result> {
  const result = nativeApi.readFile(url, branch, path);
  return Result.fromNative(result);
}