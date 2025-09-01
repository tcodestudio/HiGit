// 文件信息接口
export interface FileInfo {
  id: number;
  parentId: number;
  name: string;
  isDirectory: boolean;
  size?: number;
  extension?: string;
  path: string;
}
