import { md5String } from "../utils/Utils";
import { hilog } from "@kit.PerformanceAnalysisKit";

export class RepoItem {
  url: string;
  id: string = "";
  name: string = "";
  provider: string = "";
  activity: string = "";
  branch: string = "";
  branches: number = 0;
  tags: number = 0;
  commits: number = 0;
  time: number = 0;

  constructor(url: string) {
    this.url = url;
    this.id = md5String(this.url);
    this.parseUrl();
  }

  /**
   * 解析 Git URL，提取 provider 和 name
   * 支持格式：
   * - https://github.com/username/repo.git
   * - git@github.com:username/repo.git
   */
  private parseUrl(): void {
    if (!this.url || this.url.trim() === "") {
      return;
    }

    let url = this.url.trim();

    // 移除 .git 后缀
    if (url.endsWith('.git')) {
      url = url.substring(0, url.length - 4);
    }

    // 处理 SSH 格式 (git@host:path)
    if (url.startsWith('git@')) {
      const sshMatch = url.match(/git@([^:]+):(.+)/);
      if (sshMatch) {
        const host = sshMatch[1];
        const path = sshMatch[2];

        // 提取 provider (host 的第一部分)
        this.provider = host.split('.')[0];

        // 提取 name (路径的最后一部分)
        const pathParts = path.split('/');
        this.name = pathParts[pathParts.length - 1];
      }
      return;
    }

    // 处理 HTTPS 格式 (https://host/path)
    if (url.startsWith('http://') || url.startsWith('https://')) {
      try {
        // 移除协议部分
        let urlWithoutProtocol = url;
        if (url.startsWith('https://')) {
          urlWithoutProtocol = url.substring(8);
        } else if (url.startsWith('http://')) {
          urlWithoutProtocol = url.substring(7);
        }

        // 分离主机名和路径
        const slashIndex = urlWithoutProtocol.indexOf('/');
        if (slashIndex !== -1) {
          const host = urlWithoutProtocol.substring(0, slashIndex);
          const path = urlWithoutProtocol.substring(slashIndex);

          // 提取 provider (host 的第一部分)
          this.provider = host.split('.')[0];

          // 提取 name (路径的最后一部分)
          const pathParts = path.split('/').filter(part => part.length > 0);
          if (pathParts.length > 0) {
            this.name = pathParts[pathParts.length - 1];
          }
        } else {
          // 只有主机名，没有路径
          this.provider = urlWithoutProtocol.split('.')[0];
        }
      } catch (error) {
        hilog.error(0x0000, "appTag", error)
      }
      return;
    }

  }
}