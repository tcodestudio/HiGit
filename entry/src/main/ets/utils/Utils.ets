import { cryptoFramework } from '@kit.CryptoArchitectureKit';
import { buffer } from '@kit.ArkTS';

/**
 * 获取字符串的MD5哈希值（十六进制字符串格式）
 * @param content 要计算MD5的字符串
 * @returns MD5哈希值的十六进制字符串
 */
export function md5String(content: string): string {
  let md = cryptoFramework.createMd("MD5");
  md.updateSync({ data: new Uint8Array(buffer.from(content, 'utf-8').buffer) });
  let mdResult = md.digestSync();

  // 将字节数组转换为十六进制字符串
  let hexString = '';
  for (let i = 0; i < mdResult.data.length; i++) {
    let hex = mdResult.data[i].toString(16);
    // 确保每个字节都是两位十六进制数
    if (hex.length === 1) {
      hex = '0' + hex;
    }
    hexString += hex;
  }

  return hexString;
}

/**
 * 对分支进行排序，优先显示常见的主分支
 * @param branches 原始分支列表
 * @returns 排序后的分支列表
 */
export function sortBranches(branches: Array<string>): Array<string> {
  // 定义优先级顺序：main > master > develop > dev > 其他
  const priorityBranches = ['main', 'master', 'develop', 'dev', 'staging', 'test', 'hotfix', 'release'];

  return branches.sort((a, b) => {
    const aIndex = priorityBranches.indexOf(a);
    const bIndex = priorityBranches.indexOf(b);

    // 如果两个分支都在优先级列表中，按优先级排序
    if (aIndex !== -1 && bIndex !== -1) {
      return aIndex - bIndex;
    }

    // 如果只有 a 在优先级列表中，a 优先
    if (aIndex !== -1 && bIndex === -1) {
      return -1;
    }

    // 如果只有 b 在优先级列表中，b 优先
    if (aIndex === -1 && bIndex !== -1) {
      return 1;
    }

    // 如果都不在优先级列表中，按字母顺序排序
    return a.localeCompare(b);
  });
}

/**
 * 格式化时间戳为日期时间字符串
 * @param time Unix 时间戳（秒）
 * @returns 格式化的日期时间字符串
 */
export function formatTimestamp(time: number): string {
  try {
    const date = new Date(time * 1000);
    return date.toLocaleString('zh-CN');
  } catch (error) {
    console.error('格式化时间戳失败:', error);
    return '未知时间';
  }
}

/**
 * 格式化时间戳为相对时间字符串（如：刚刚、几分钟前、几小时前等）
 * 超过1周的时间显示具体日期
 * @param time Unix 时间戳（秒）
 * @returns 相对时间字符串或具体日期
 */
export function formatTimestampLocal(time: number): string {
  try {
    const now = Math.floor(Date.now() / 1000); // 当前时间戳（秒）
    const diff = now - time; // 时间差（秒）

    if (diff < 0) {
      return '未来时间';
    }

    if (diff < 60) {
      return '刚刚';
    }

    if (diff < 3600) {
      const minutes = Math.floor(diff / 60);
      if (minutes === 1) {
        return '1分钟前';
      }
      return `${minutes}分钟前`;
    }

    if (diff < 86400) {
      const hours = Math.floor(diff / 3600);
      if (hours === 1) {
        return '1小时前';
      }
      return `${hours}小时前`;
    }

    if (diff < 604800) { // 7天 = 7 * 24 * 3600
      const days = Math.floor(diff / 86400);
      if (days === 1) {
        return '1天前';
      }
      return `${days}天前`;
    }

    // 超过1周的时间显示具体日期
    const date = new Date(time * 1000);
    const currentYear = new Date().getFullYear();
    const timeYear = date.getFullYear();

    if (timeYear === currentYear) {
      // 今年内显示月日
      return `${date.getMonth() + 1}月${date.getDate()}日`;
    } else {
      // 跨年显示年月日
      return `${timeYear}年${date.getMonth() + 1}月${date.getDate()}日`;
    }

  } catch (error) {
    console.error('格式化相对时间失败:', error);
    return '未知时间';
  }
}

/**
 * 智能截断URL，保持可读性
 * @param url 原始URL
 * @param maxLength 最大长度
 * @returns 截断后的URL
 */
export function truncateUrl(url: string, maxLength: number = 50): string {
  if (!url || url.length <= maxLength) {
    return url;
  }

  // 如果是SSH格式 (git@host:path)
  if (url.startsWith('git@')) {
    const parts = url.split(':');
    if (parts.length === 2) {
      const host = parts[0]; // git@host
      const path = parts[1]; // path

      if (host.length + 3 + path.length <= maxLength) {
        return url;
      }

      // 截断路径部分，保留主机名
      const availableLength = maxLength - host.length - 3; // 3 for '...'
      if (availableLength > 0) {
        return `${host}:...${path.substring(path.length - availableLength)}`;
      } else {
        return `${host}:...`;
      }
    }
  }

  // 如果是HTTPS/HTTP格式
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
        const hostname = urlWithoutProtocol.substring(0, slashIndex);
        const pathname = urlWithoutProtocol.substring(slashIndex);
        const protocol = url.startsWith('https://') ? 'https://' : 'http://';

        // 计算可用长度
        const usedLength = protocol.length + hostname.length + 3; // 3 for '...'
        const availableLength = maxLength - usedLength;

        if (availableLength > 0) {
          // 保留协议和主机名，截断路径
          return `${protocol}${hostname}...${pathname.substring(pathname.length - availableLength)}`;
        } else {
          return `${protocol}${hostname}...`;
        }
      } else {
        // 只有主机名，没有路径
        const protocol = url.startsWith('https://') ? 'https://' : 'http://';
        return `${protocol}${urlWithoutProtocol}...`;
      }
    } catch (error) {
      // 如果URL解析失败，使用简单截断
      return url.substring(0, maxLength - 3) + '...';
    }
  }

  // 其他格式，简单截断
  return url.substring(0, maxLength - 3) + '...';
}

/**
 * 获取URL的显示名称（用于工具提示等）
 * @param url 原始URL
 * @returns 显示名称
 */
export function getUrlDisplayName(url: string): string {
  if (!url) {
    return '';
  }

  // 移除.git后缀
  let displayUrl = url;
  if (displayUrl.endsWith('.git')) {
    displayUrl = displayUrl.substring(0, displayUrl.length - 4);
  }

  return displayUrl;
}

/**
 * 格式化文件大小
 * @param size 文件大小
 * @returns
 */
export function formatFileSize(size: number): string {
  if (size === 0) {
    return '';
  }
  if (size < 1024) {
    return `${size} B`;
  }
  if (size < 1024 * 1024) {
    return `${(size / 1024).toFixed(1)} KB`;
  }
  return `${(size / (1024 * 1024)).toFixed(1)} MB`;
}

/**
 * 验证 Git 仓库地址格式
 * @param url 要验证的 URL
 * @returns 是否为有效的 Git 仓库地址
 */
export function validateGitUrl(url: string): boolean {
  // 检查是否以 .git 结尾
  if (!url.endsWith('.git')) {
    return false;
  }

  // 检查 HTTPS 格式
  if (url.startsWith('https://') || url.startsWith('http://')) {
    // HTTPS 格式应该包含域名和路径
    const urlPattern = /^https?:\/\/[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}\/[a-zA-Z0-9._-]+\/[a-zA-Z0-9._-]+\.git$/;
    return urlPattern.test(url);
  }

  // 检查 SSH 格式 (git@host:path.git)
  if (url.startsWith('git@')) {
    const sshPattern = /^git@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}:[a-zA-Z0-9._-]+\/[a-zA-Z0-9._-]+\.git$/;
    return sshPattern.test(url);
  }

  // 检查 Git 协议格式 (git://host/path.git)
  if (url.startsWith('git://')) {
    const gitPattern = /^git:\/\/[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}\/[a-zA-Z0-9._-]+\/[a-zA-Z0-9._-]+\.git$/;
    return gitPattern.test(url);
  }

  // 检查 SSH 协议格式 (ssh://user@host/path.git)
  if (url.startsWith('ssh://')) {
    const sshUrlPattern =
      /^ssh:\/\/[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}\/[a-zA-Z0-9._-]+\/[a-zA-Z0-9._-]+\.git$/;
    return sshUrlPattern.test(url);
  }

  return false;
}