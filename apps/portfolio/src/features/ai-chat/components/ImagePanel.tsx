"use client";

import type { DragEvent, FormEvent } from "react";
import { useState } from "react";
import { FileImage, ImageIcon, Loader2, UploadCloud, X } from "lucide-react";

import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";

type ImagePanelProps = {
  selectedFile: File | null;
  imagePreview: string | null;
  imageResult: string | null;
  imageConfidence: number | null;
  imageError: string | null;
  uploading: boolean;
  onFileChange: (file: File | null) => void;
  onUpload: (event: FormEvent<HTMLFormElement>) => void;
};

export function ImagePanel({
  selectedFile,
  imagePreview,
  imageResult,
  imageConfidence,
  imageError,
  uploading,
  onFileChange,
  onUpload,
}: ImagePanelProps) {
  const [isDragging, setIsDragging] = useState(false);
  const confidencePercent =
    typeof imageConfidence === "number"
      ? Math.max(0, Math.min(100, Math.round(imageConfidence * 100)))
      : null;

  function formatFileSize(file: File) {
    if (file.size < 1024 * 1024) {
      return `${Math.max(1, Math.round(file.size / 1024))} KB`;
    }

    return `${(file.size / 1024 / 1024).toFixed(1)} MB`;
  }

  function handleDrop(event: DragEvent<HTMLDivElement>) {
    event.preventDefault();
    setIsDragging(false);

    const file = event.dataTransfer.files?.[0] ?? null;
    if (file) {
      onFileChange(file);
    }
  }

  return (
    <form className="flex min-h-[34rem] flex-col gap-5 p-4" onSubmit={onUpload}>
      <div
        className={cn(
          "rounded-lg border border-dashed bg-background/80 p-6 text-center transition-colors",
          isDragging
            ? "border-primary bg-muted/70"
            : "hover:bg-muted/50"
        )}
        onDragEnter={(event) => {
          event.preventDefault();
          setIsDragging(true);
        }}
        onDragOver={(event) => event.preventDefault()}
        onDragLeave={() => setIsDragging(false)}
        onDrop={handleDrop}
      >
        <input
          id="chat-image-upload"
          className="sr-only"
          type="file"
          accept="image/*"
          onChange={(event) => onFileChange(event.target.files?.[0] ?? null)}
        />
        <label
          htmlFor="chat-image-upload"
          className="flex min-h-36 cursor-pointer flex-col items-center justify-center gap-3"
        >
          <UploadCloud className="size-8 text-muted-foreground" />
          <span className="text-sm font-medium">
            Drop an image here or choose a file
          </span>
          <span className="text-xs text-muted-foreground">
            PNG, JPG, GIF, or WebP. Max 5 MB.
          </span>
        </label>
      </div>

      {selectedFile && (
        <div className="flex items-center justify-between gap-3 rounded-lg border bg-background px-3 py-2">
          <div className="flex min-w-0 items-center gap-3">
            <FileImage className="size-4 flex-none text-muted-foreground" />
            <div className="min-w-0">
              <p className="truncate text-sm font-medium">{selectedFile.name}</p>
              <p className="text-xs text-muted-foreground">
                {formatFileSize(selectedFile)}
              </p>
            </div>
          </div>
          <Button
            type="button"
            variant="ghost"
            size="icon"
            className="size-8 flex-none"
            onClick={() => onFileChange(null)}
          >
            <X className="size-4" />
            <span className="sr-only">Remove image</span>
          </Button>
        </div>
      )}

      {imagePreview && (
        <div className="overflow-hidden rounded-lg border bg-background">
          {/* eslint-disable-next-line @next/next/no-img-element */}
          <img
            src={imagePreview}
            alt={selectedFile?.name || "Selected image"}
            className="max-h-80 w-full object-contain"
          />
        </div>
      )}

      {imageError && (
        <div className="rounded-lg border border-destructive/40 bg-background px-4 py-3 text-sm text-destructive">
          {imageError}
        </div>
      )}

      {imageResult && (
        <div className="rounded-lg border bg-background px-4 py-3">
          <p className="text-xs uppercase text-muted-foreground">Recognition</p>
          <div className="mt-1 flex flex-wrap items-end justify-between gap-2">
            <p className="text-lg font-semibold">{imageResult}</p>
            {confidencePercent !== null && (
              <p className="text-sm text-muted-foreground">
                {confidencePercent}% confidence
              </p>
            )}
          </div>
          {confidencePercent !== null && (
            <div className="mt-3 h-2 overflow-hidden rounded-full bg-muted">
              <div
                className="h-full rounded-full bg-primary"
                style={{ width: `${confidencePercent}%` }}
              />
            </div>
          )}
        </div>
      )}

      <Button type="submit" className="gap-2" disabled={!selectedFile || uploading}>
        {uploading ? (
          <Loader2 className="size-4 animate-spin" />
        ) : (
          <ImageIcon className="size-4" />
        )}
        Recognize
      </Button>
    </form>
  );
}
